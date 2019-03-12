#
# themis/realtime/loop.jl --
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
runloop(cam, [[beforewait,] preprocessing,] processing) -> count
```

runs a loop driven by the acquisition of images by the camera `cam`.  The
operations to perform for each iteration of the loop are specified by up to 3
methods:

- Method `beforewait` is called just before waiting for the next image as:

  ```julia
  beforewait(count) -> boolean
  ```

  where `count` is the actual number of acquired images, this value may be less
  than zero when no images have been acquired yet or if the next image is to be
  skipped (see keyword `skip`).  The `beforewait` method shall return a boolean
  value indicating whether to continue the loop.  The most simple `beforewait`
  argument is thus the inline method shon in the following example:

  ```julia
  runloop(cam, count -> true, ...)
  ```

  which is the default method provided when the `beforewait` argument is
  omitted.

- Method `preprocessing` is called just after a new image has been acquired as:

  ```julia
  preprocessing(raw) -> img
  ```

  where `raw` is the image delivered by the camera and the result `img` is the
  pre-processed image.  The preprocessing can perform bias and gain
  corrections.  See [`Themis.preprocessing`](@ref) or
  [`Themis.preprocessing!`](@ref) for examples.

- Method `processing` is called for each pre-processed image as:

  ```julia
  processing(img, count, ticks) -> boolean
  ```

  where `img` is the pre-processed image, `count` is the actual number of
  acquired images (should be greater or equal 1) and `ticks` is the date of the
  image.  The `processing` method shall return a boolean value indicating
  whether to continue the loop.  The provided image `img` is an instance of
  `DenseMatrix{T}` where `T` is the same floating point type as `gain` and
  `bias`.

The `preprocessing` method can be replaced by two arguments:

```julia
runloop(cam, [[beforewait,] bias, gain,] processing) -> count
```

where `bias` and `gain` are matrices which are used to pre-process the raw
image as follows:

```julia
img .= (raw .- bias).*gain
```

where `img` is allocated once for maximum efficiency.

The following keywords are available:

- Keyword `skip` can be used to specify a number of images to skip before
  starting processing (default: `0`).  This may be useful to discard first
  images that may contain garbage or may be saturated if the camera has no
  shutter.

- Keyword `drop` specifies whether to drop unprocessed images older than the
  last one (default: `false`).  This may be useful to avoid jitter or lag when
  the processing of images is not fast enough for the camera frame rate.

- Keyword `timeout` can be used to specify the maximum time (in seconds) to
  wait for a new image.

- Keyword `rawpixeltype` can be used to specify the pixel type of the acquired
  images.

- If no pre-processing method nor pre-preprocessing parameters are specified,
  keyword `pixeltype` can be used to specify the pixel type of the
  pre-processed images which, in that case, are simple copies, possibly with
  type conversion, of the acquired images (default: `pixeltype=rawpixeltype`).

- Keyword `nbufs` specifies the number of frame buffers to use for continuous
  acquisition (default: `4`).

"""
function runloop(cam::ScientificCamera,
                 beforewait::Function,
                 preprocessing::Function,
                 processing::Function;
                 skip::Integer = 0,
                 drop::Bool = false,
                 timeout::Real = defaulttimeout(cam),
                 rawpixeltype::Type{<:Integer} = getcapturebitstype(cam),
                 nbufs::Integer = 4) where {T<:AbstractFloat}
    nbufs ≥ 2 || throw(ArgumentError("invalid number of buffers"))
    skip ≥ 0 || throw(ArgumentError("invalid number of images to skip"))
    timeout > zero(timeout) || throw(ArgumentError("invalid timeout"))
    start(cam, rawpixeltype, nbufs)
    count = -Int(skip)
    while true
        try
            # Perform operations before the next image, then wait for the next
            # image.
            if ! beforewait(count)
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
                img = preprocessing(raw)
                release(cam)
                if ! processing(img, count, ticks)
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

# This version provide a `beforewait` callback which does nothing.
function runloop(cam::ScientificCamera,
                 bias::DenseMatrix{T},
                 gain::DenseMatrix{T},
                 processing::Function;
                 kwds...) where {T<:AbstractFloat}
    runloop(cam,
            count -> true, # beforewait
            bias, gain,    # for pre-processing
            processing;
            kwds...)
end

function runloop(cam::ScientificCamera,
                 beforewait::Function,
                 bias::DenseMatrix{T},
                 gain::DenseMatrix{T},
                 processing::Function;
                 kwds...) where {T<:AbstractFloat}
    @assert (dims = size(bias)) == size(gain)
    img = Array{T,2}(undef, dims)
    runloop(cam,
            beforewait,
            raw -> Themis.preprocessing!(img, raw, bias, gain),
            processing;
            kwds...)
end

function runloop(cam::ScientificCamera,
                 processing::Function;
                 rawpixeltype::Type{<:Integer} = getcapturebitstype(cam),
                 pixeltype::Type{<:Real} = rawpixeltype,
                 kwds...) where {T<:AbstractFloat}
    roi = getroi(cam)
    dims = (roi.width, roi.height)
    img = Array{pixeltype,2}(undef, dims)
    runloop(cam,
            count -> true, # beforewait
            raw -> Themis.preprocessing!(img, raw),
            processing;
            kwds...)
end

#beforewait(count::Int) = true
#processing(img::Matrix{T}, count::Int, ticks) where {T<:AbstractFloat} = true

function closeloop(cam::ScientificCamera,
                   dm::Alpao.DeformableMirror,
                   G::AbstractArray{Cdouble},
                   preprocessing::Function;
                   cref::Vector{Cdouble},
                   xyref::Matrix{Cdouble},
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

    function beforewait(cnt::Int)
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

    function processing(img::DenseMatrix{T},
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
    runloop(cam, beforewait, preprocessing, processing; kwds...)
    history || println()
    debug || send(dm, cref)
end

# Same as `norm(x,Inf)`?
norminf(x) = (ex = extrema(x); @inbounds max(-ex[1], ex[2]))
