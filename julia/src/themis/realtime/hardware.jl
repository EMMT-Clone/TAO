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

# This version provide a `beforeimage` callback which does nothing.
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
                   cref::Vector{Cdouble},
                   xyref::Matrix{Cdouble},
                   gain::AbstractMatrix,
                   bias::AbstractMatrix,
                   order::Integer = 2,
                   mu::Real = 2.0,
                   gamma::Real = 1.0,
                   sigma::Real = 0.03,
                   alpha::Real = 0.1,
                   nloops::Integer=1000,
                   msk::AbstractArray{Bool} = Themis.DM_SHAPE,
                   history::Bool = false,
                   debug::Bool = false,
                   kwds...)

    local cmd, crnd, ccor, crms, perr

    @assert 0 < sigma
    @assert 0 ≤ alpha ≤ 1
    @assert 0 ≤ gamma ≤ 1
    P = Themis.subsamplingmatrix(msk; flat=true);
    q = Themis.matrices4map(G, msk, order)
    nacts = length(dm)
    crnd = Cdouble(sigma).*randn(nacts)
    beta = sqrt(1.0 - Cdouble(alpha)^2)
    ccor = zeros(Cdouble, nacts)
    perr = Cdouble(0)

    function before(cnt::Int)
        cmd = cref .+ (crnd .- ccor)
        crms = norm(cmd)/sqrt(nacts)
        if norminf(cmd) > 1.0
            println("\ndiverging...")
            return false
        else
            send(dm, cmd)
            return true
        end
    end

    function after(img::DenseMatrix{T},
                   cnt::Int,
                   ticks) where {T<:AbstractFloat}
        d = Centroiding.fit(img, xyref) .- xyref;
        ccor += gamma.*P*Themis.solvemapproblem(q..., mu, d);
        cerr1 = norm(crnd .- ccor)/sqrt(nacts) - perr
        # Combine old random perturbation (recentered to avoid drifting of
        # piston) with new random values.  FIXME: Recentering changes (a bit)
        # the variance: fix it!
        crnd .= (Cdouble(beta).*(crnd .- mean(crnd))
                 .+ Cdouble(alpha*sigma).*randn(nacts))
        cerr2 = norm(crnd .- ccor)/sqrt(nacts)
        cerr3 = norm(crnd)/sqrt(nacts)
        if history
            @printf("%5d %12.6f %12.6f %12.6f %12.3f\n", cnt,
                    cerr1, cerr2, crms, cerr3)
        else
            @printf("\r%5d %12.6f %12.6f %12.6f %12.3f", cnt,
                    cerr1, cerr2, crms, cerr3)
            flush(stdout)
        end
        perr = cerr2
        return cnt < nloops
    end

    println("Iter. ΔError       Resid. (RMS) Comm. (RMS)  Mean Energy")
    println("----- ------------ ------------ ------------ ------------")
    runloop(cam, gain, bias, before, after; kwds...)
    history || println()
    debug || send(dm, cref)
end

# Same as `norm(x,Inf)`?
norminf(x) = (ex = extrema(x); @inbounds max(-ex[1], ex[2]))
