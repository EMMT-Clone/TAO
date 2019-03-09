#
# themis/realtime/hardware.jl --
#
# Methods related to the hardware of the Themis adaptive optics system (the
# deformable mirror, the wavefront sensor camera, etc.).
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018-2019, Éric Thiébaut & Michel Tallon.
#

# FIXME: This a hack which sould be removed once Phoenix package has been
#        fixed.
function Phoenix.read(cam::Phoenix.Camera, ::Type{T}, num::Int;
                      skip::Integer = 0,
                      timeout::Real = defaulttimeout(cam),
                      truncate::Bool = false,
                      drop::Bool = true,
                      nbufs::Integer = 8) where {T}
    # Check arguments.
    num ≥ 1 || throw(ArgumentError("invalid number of images"))
    nbufs ≥ 2 || throw(ArgumentError("invalid number of buffers"))
    skip ≥ 0 || throw(ArgumentError("invalid number of images to skip"))
    timeout > zero(timeout) || throw(ArgumentError("invalid timeout"))

    # Start acquisition with given callback and collect images.
    imgs = Vector{Array{T,2}}(undef, num)
    cnt = 0
    start(cam, T, nbufs)
    while cnt < num
        try
            img, ticks = wait(cam, timeout, drop)
            if skip > zero(skip)
                # Skip this frame.
                skip -= one(skip)
                release(cam)
            else
                # Store this frame.
                cnt += 1
                imgs[cnt] = img
                release(cam)
            end
        catch err
            if truncate && isa(err, Phoenix.TimeoutError)
                @warn "Acquisition timeout after $cnt image(s)"
                num = cnt
                resize!(imgs, num)
            else
                abort(cam)
                rethrow(err)
            end
        end
    end

    # Stop immediately.
    abort(cam)

    # Return images.
    return imgs
end

"""
```julia
runloop(cam, gain, bias, [beforeimage,] afterimage) -> count
```

runs the real-time loop given the wavefront sensor camera `cam`, the image bias
and gain corrections and two methods `beforeimage` and `afterimage` to be
respectively called just before waiting for the next image and just after a new
image has been acquired.

Method `beforeimage` is called as:

```julia
beforeimage(count) -> boolean
```

where `count` is the actual number of acquired images, this value may be less
than zero when no images have been acquired yet or if the next image is to be
skipped (see keyword `skip`).  The `beforeimage` method shall return a boolean
value indicating whether to continue the loop.  If the `beforeimage` method
is omitted, a default one is provided which always returns true.

Method `afterimage` is called as:

```julia
afterimage(img, count, ticks) -> boolean
```

where `img` is the pre-processed image, `count` is the actual number of
acquired images (should be greater or equal 1) and `ticks` is the date of the
image.  The `afterimage` method shall return a boolean value indicating whether
to continue the loop.  The provided image `img` is an instance of
`DenseMatrix{T}` where `T` is the same floating point type as `gain` and
`bias`.

"""
function runloop(cam::Phoenix.Camera,
                 gain::DenseMatrix{T},
                 bias::DenseMatrix{T},
                 beforeimage::Function,
                 afterimage::Function;
                 skip::Integer = 10,
                 timeout::Real = defaulttimeout(cam),
                 drop::Bool = true,
                 nbufs::Integer = 8) where {T<:AbstractFloat}
    dims = size(gain)
    @assert size(bias) == dims
    nbufs ≥ 2 || throw(ArgumentError("invalid number of buffers"))
    skip ≥ 0 || throw(ArgumentError("invalid number of images to skip"))
    timeout > zero(timeout) || throw(ArgumentError("invalid timeout"))
    img = Array{T,2}(undef, dims)
    start(cam, UInt8, nbufs)
    count = -Int(skip)
    while true
        try
            # Perform operations before the next image, then wait for the next
            # image.
            if ! beforeimage(count)
                break
            end
            raw, ticks = wait(cam, timeout, drop)#::Tuple{Array{UInt8,2},Cdouble}
            count += 1
            if count ≤ 0
                # Skip this frame.
                release(cam)
            else
                # Pre-process the pixels of the last frame buffer then
                # release the frame buffer and apply the image processing
                # callback.
                @assert size(raw) == dims
                @inbounds @simd for k in 1:length(img)
                    img[k] = (raw[k] - bias[k])*gain[k]
                end
                release(cam)
                if ! afterimage(img, count, ticks)
                    break
                end
            end
        catch err
            if isa(err, Phoenix.TimeoutError)
                @warn "Acquisition timeout after $count image(s)"
            else
                abort(cam)
                rethrow(err)
            end
        end
    end

    # Abort acquisition.
    abort(cam)
    return count
end

function runloop(cam::Phoenix.Camera,
                 gain::DenseMatrix{T},
                 bias::DenseMatrix{T},
                 afterimage::Function;
                 kwds...) where {T<:AbstractFloat}
    runloop(cam, gain, bias, (count) -> true, afterimage; kwds...)
end

#beforeimage(count::Int) = true
#afterimage(img::Matrix{T}, count::Int, ticks) where {T<:AbstractFloat} = true

function closeloop(cam::Phoenix.Camera,
                   dm::Alpao.DeformableMirror,
                   G::AbstractArray{Cdouble};
                   skip::Integer = 10,
                   timeout::Real = defaulttimeout(cam),
                   drop::Bool = true,
                   nbufs::Integer = 8,
                   cref::Vector{Cdouble},
                   xyref::Matrix{Cdouble},
                   gain::AbstractMatrix,
                   bias::AbstractMatrix,
                   order::Integer = 2,
                   mu::Real = 2.0,
                   gamma::Real = 1.0,
                   amp::Real = 0.03,
                   nloops::Integer=1000,
                   msk::AbstractArray{Bool} = Themis.DM_SHAPE)

    local ctot, crnd, ccor

    @assert 0 < gamma ≤ 1
    P = ThemisUtils.subsamplingmatrix(msk; flat=true);
    q = ThemisUtils.matrices4map(G, msk, order)
    ncmds = length(dm)
    crnd = amp.*randn(ncmds)
    ccor = zeros(Cdouble, ncmds)

    function before(cnt::Int)
        ctot = cref .+ (crnd .- ccor)
        crms = norm(ctot)/sqrt(ncmds)
        if minimum(ctot) < -0.9 || maximum(ctot) > 0.9
            println("\ndiverging...")
            return false
        else
            send(dm, ctot)
            return true
        end
    end

    function after(img::DenseMatrix{T},
                   cnt::Int,
                   ticks) where {T<:AbstractFloat}
        xy = Centroiding.fit(img, xyref[1,:], xyref[2,:]);
        d = hcat(xy[1] .- xyref[1,:], xy[2] .- xyref[2,:])';
        ccor += gamma.*P*ThemisUtils.solvemapproblem(q..., mu, d);
        cerr1 = crnd .- ccor
        crnd += amp.*randn(ncmds) .- mean(crnd) # Markov chain
        cerr2 = crnd .- ccor
        @printf("\r%5d %12.6f %12.6f %12.6f %12.3f", iter,
                norm(cerr1)/sqrt(ncmds),
                norm(cerr2)/sqrt(ncmds),
                crms,
                norm(crnd)/sqrt(ncmds))
        flush(stdout)
        return cnt < nloops
    end

    println("Iter. Error (RMS)  Delayed Err. Comm. (RMS)  Mean Energy")
    println("----- ------------ ------------ ------------ ------------")
    runloop(cam, gain, bias, before, after; kwds...)
    println()
end
