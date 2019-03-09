#
# Centroiding.jl -
#
# Algorithms for estimating the positions of spots in images.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018-2019, Éric Thiébaut and Michel Tallon.
#

module Centroiding

#export
#    MemoryLessSeparableShape,
#    NonSeparableShape,
#    SeparableShape,
#    ShapeModel,
#    cost,
#    cubicspline

using OptimPackNextGen.Powell.Newuoa

abstract type ShapeModel end

struct NonSeparableShape{F<:Function} <: ShapeModel
    fn::F        # 2D shape function
    sup::Cdouble # size of support
end

struct SeparableShape{F<:Function} <: ShapeModel
    fn::F                # 1D shape function
    sup::Cdouble         # size of support
    wrk::Vector{Cdouble} # workspace
    SeparableShape{F}(fn::F, sup::Cdouble, len::Integer=0) where {F<:Function} =
        new{F}(fn, sup, Vector{Cdouble}(undef, len))
end

struct MemoryLessSeparableShape{F<:Function} <: ShapeModel
    fn::F        # 1D shape function
    sup::Cdouble # size of support
end

NonSeparableShape(fn::F, sup::Real) where {F<:Function} =
    NonSeparableShape{F}(fn, Cdouble(sup))

SeparableShape(fn::F, sup::Real) where {F<:Function} =
    SeparableShape{F}(fn, Cdouble(sup))

MemoryLessSeparableShape(fn::F, sup::Real) where {F<:Function} =
    MemoryLessSeparableShape{F}(fn, Cdouble(sup))

(shape::NonSeparableShape)(δx::Cdouble, δy::Cdouble) = shape.fn(δx,δy)
(shape::SeparableShape)(δx) = shape.fn(δx::Cdouble)
(shape::MemoryLessSeparableShape)(δx) = shape.fn(δx::Cdouble)

support(shape::ShapeModel) = Cdouble(shape.sup)
support(shape::ShapeModel, siz::Real) = support(shape)*Cdouble(siz)

frac(::Type{T}, num::Integer, den::Integer) where {T<:AbstractFloat} =
    T(num)/T(den)

square(x) = x*x

cube(x) = x*x*x

const CUBICSPLINE_FWHM = (4 + 8*cos((π + atan(3*sqrt(7)))/3))/3;

function cubicspline(x::T) where {T<:AbstractFloat}
    a = abs(x)
    return (a ≥ 2 ? zero(T) :
            a ≥ 1 ? cube(T(2) - a)*frac(T,1,6) :
            (frac(T,1,2)*a - T(1))*a*a + frac(T,2,3))
end

const memorylesscubicsplinemodel = MemoryLessSeparableShape(cubicspline, 4)
const cubicsplinemodel = SeparableShape(cubicspline, 4)

function _get_workspace(shape::SeparableShape, len::Int)
    wrk = shape.wrk
    length(wrk) ≥ len || resize!(wrk, len)
    return wrk
end

"""
```julia
cost([wgt,] dat, shape, siz, x, y)
```

yields the value of the objective function for a spot whose footprint and size
are given by `shape` and `siz` and assumed position is `(x,y)`. Arguments `dat`
`wgt` are the data and their respective weights.

"""
function cost(wgt::AbstractMatrix{<:AbstractFloat},
              dat::AbstractMatrix{<:AbstractFloat},
              shape::ShapeModel,
              siz::Cdouble,  # size of spot
              x::Cdouble,
              y::Cdouble) :: Cdouble
    @assert ! Base.has_offset_axes(wgt, dat)
    @assert size(wgt) == size(dat)
    imin, imax, jmin, jmax = _getbbox(size(dat), support(shape, siz), x, y)
    return _cost(wgt, dat, imin, imax, jmin, jmax, shape, 1/siz, x, y)
end

function cost(dat::AbstractMatrix{<:AbstractFloat},
              shape::ShapeModel,
              siz::Cdouble,  # size of spot
              x::Cdouble,
              y::Cdouble) :: Cdouble
    @assert ! Base.has_offset_axes(dat)
    imin, imax, jmin, jmax = _getbbox(size(dat), support(shape, siz), x, y)
    return _cost(dat, imin, imax, jmin, jmax, shape, 1/siz, x, y)
end

function cost(wgt::AbstractMatrix{<:AbstractFloat},
              dat::AbstractMatrix{<:AbstractFloat},
              shape::ShapeModel,
              siz::Real,
              x::Real,
              y::Real) :: Cdouble
    return cost(wgt, dat, shape, Cdouble(siz), Cdouble(x), Cdouble(y))
end

function cost(dat::AbstractMatrix{<:AbstractFloat},
              shape::ShapeModel,
              siz::Real,
              x::Real,
              y::Real) :: Cdouble
    return cost(dat, shape, Cdouble(siz), Cdouble(x), Cdouble(y))
end

function _cost(wgt::AbstractMatrix{<:AbstractFloat},
               dat::AbstractMatrix{<:AbstractFloat},
               imin::Int, imax::Int,
               jmin::Int, jmax::Int,
               shape::MemoryLessSeparableShape,
               scl::Cdouble,  # pixel scale factor
               x::Cdouble,
               y::Cdouble) :: Cdouble
    f = shape.fn
    a, b = zero(Cdouble), zero(Cdouble)
    @inbounds for j in jmin:jmax
        fy = f((Cdouble(j) - y)*scl)
        @simd for i in imin:imax
            fx = f((Cdouble(i) - x)*scl)
            (δa, δb) = _cost_incr(wgt[i,j], dat[i,j], fx*fy)
            a += δa
            b += δb
        end
    end
    return _cost_result(a, b)
end

function _cost(dat::AbstractMatrix{<:AbstractFloat},
               imin::Int, imax::Int,
               jmin::Int, jmax::Int,
               shape::MemoryLessSeparableShape,
               scl::Cdouble,  # pixel scale factor
               x::Cdouble,
               y::Cdouble) :: Cdouble
    f = shape.fn
    a, b = zero(Cdouble), zero(Cdouble)
    @inbounds for j in jmin:jmax
        fy = f((Cdouble(j) - y)*scl)
        @simd for i in imin:imax
            fx = f((Cdouble(i) - x)*scl)
            (δa, δb) = _cost_incr(dat[i,j], fx*fy)
            a += δa
            b += δb
        end
    end
    return _cost_result(a, b)
end

# FIXME: Beware this cannot be threaded (yet).  Unless workspace is a
# vector of vectors.
function _cost(wgt::AbstractMatrix{<:AbstractFloat},
               dat::AbstractMatrix{<:AbstractFloat},
               imin::Int, imax::Int,
               jmin::Int, jmax::Int,
               shape::SeparableShape,
               scl::Cdouble,  # pixel scale factor
               x::Cdouble,
               y::Cdouble) :: Cdouble
    f = shape.fn
    off = 1 - imin
    wrk = _get_workspace(shape, imax + off)
    @inbounds @simd for i in imin:imax
        wrk[i + off] = f((Cdouble(i) - x)*scl)
    end
    a, b = zero(Cdouble), zero(Cdouble)
    @inbounds for j in jmin:jmax
        fy = f((Cdouble(j) - y)*scl)
        @simd for i in imin:imax
            fx = wrk[i + off]
            (δa, δb) = _cost_incr(wgt[i,j], dat[i,j], fx*fy)
            a += δa
            b += δb
        end
    end
    return _cost_result(a, b)
end

function _cost(dat::AbstractMatrix{<:AbstractFloat},
               imin::Int, imax::Int,
               jmin::Int, jmax::Int,
               shape::SeparableShape,
               scl::Cdouble,  # pixel scale factor
               x::Cdouble,
               y::Cdouble) :: Cdouble
    f = shape.fn
    off = 1 - imin
    wrk = _get_workspace(shape, imax + off)
    @inbounds @simd for i in imin:imax
        wrk[i + off] = f((Cdouble(i) - x)*scl)
    end
    a, b = zero(Cdouble), zero(Cdouble)
    @inbounds for j in jmin:jmax
        fy = f((Cdouble(j) - y)*scl)
        @simd for i in imin:imax
            fx = wrk[i + off]
            (δa, δb) = _cost_incr(dat[i,j], fx*fy)
            a += δa
            b += δb
        end
    end
    return _cost_result(a, b)
end

function _cost(wgt::AbstractMatrix{<:AbstractFloat},
               dat::AbstractMatrix{<:AbstractFloat},
               imin::Int, imax::Int,
               jmin::Int, jmax::Int,
               shape::NonSeparableShape,
               scl::Cdouble,  # pixel scale factor
               x::Cdouble,
               y::Cdouble) :: Cdouble
    f = shape.fn
    a, b = zero(Cdouble), zero(Cdouble)
    @inbounds for j in jmin:jmax
        δy = (Cdouble(j) - y)*scl
        @simd for i in imin:imax
            δx = (Cdouble(i) - x)*scl
            (δa, δb) = _cost_incr(wgt[i,j], dat[i,j], f(δx,δy))
            a += δa
            b += δb
        end
    end
    return _cost_result(a, b)
end

function _cost(dat::AbstractMatrix{<:AbstractFloat},
               imin::Int, imax::Int,
               jmin::Int, jmax::Int,
               shape::NonSeparableShape,
               scl::Cdouble,  # pixel scale factor
               x::Cdouble,
               y::Cdouble) :: Cdouble
    f = shape.fn
    a, b = zero(Cdouble), zero(Cdouble)
    @inbounds for j in jmin:jmax
        δy = (Cdouble(j) - y)*scl
        @simd for i in imin:imax
            δx = (Cdouble(i) - x)*scl
            (δa, δb) = _cost_incr(dat[i,j], f(δx,δy))
            a += δa
            b += δb
        end
    end
    return _cost_result(a, b)
end

"""
    _cost_result(a, b)

yields the cost given the integrated quantities `a` and `b`.

"""
_cost_result(a::Cdouble, b::Cdouble) =
    (b > 0 && a > 0) ? -b*b/a : zero(Cdouble)

"""
    _cost_incr([w,] d, s) -> (δa, δb)

yields the increments of the cost integrated quantities `a` and `b` given the
weight `w`, the data `d` and the shape `s`.

"""
@inline _cost_incr(w::Cdouble, d::Cdouble, s::Cdouble) =
    (ws = w*s; return (s*ws, d*ws))

@inline _cost_incr(d::Cdouble, s::Cdouble) = (s*s, d*s)

@inline _cost_incr(w::AbstractFloat, d::AbstractFloat, s::AbstractFloat) =
    _cost_incr(Cdouble(w), Cdouble(d), Cdouble(s))

@inline _cost_incr(d::AbstractFloat, s::AbstractFloat) =
    _cost_incr(Cdouble(d), Cdouble(s))

#------------------------------------------------------------------------------
# A first possibility is to globally sum the costs for each spot.

function cost(wgt::AbstractMatrix{<:AbstractFloat},
              dat::AbstractMatrix{<:AbstractFloat},
              shape::ShapeModel,
              siz::Real,
              x::AbstractVector{<:Real},
              y::AbstractVector{<:Real}) :: Cdouble
    # The idea is that the spots do not overlap so we just do the sum of
    # the costs.
    @assert length(x) == length(y)
    s = Cdouble(0)
    for i in eachindex(x, y)
        # FIXME: use @thread
        s += cost(wgt, dat, shape, Cdouble(siz), Cdouble(x[i]), Cdouble(y[i]))
    end
    return s
end

function cost(dat::AbstractMatrix{<:AbstractFloat},
              shape::ShapeModel,
              siz::Real,
              x::AbstractVector{<:Real},
              y::AbstractVector{<:Real}) :: Cdouble
    # The idea is that the spots do not overlap so we just do the sum of
    # the costs.
    @assert length(x) == length(y)
    s = Cdouble(0)
    for i in eachindex(x, y)
        # FIXME: use @thread
        s += cost(dat, shape, Cdouble(siz), Cdouble(x[i]), Cdouble(y[i]))
    end
    return s
end

function slowfit(img::AbstractMatrix{<:AbstractFloat},
                 x0::AbstractVector{<:Real},
                 y0::AbstractVector{<:Real};
                 shape::ShapeModel = cubicsplinemodel,
                 siz::Real = 2.5,
                 rho::Tuple{Real,Real} = (3.0, 0.01))
    @assert length(x0) == length(y0)
    len = length(x0)
    x0y0 = Vector{Cdouble}(undef, 2*len)
    for i in 1:len
        x0y0[2*i - 1] = x0[i]
        x0y0[2*i]     = y0[i]
    end
    rep = Newuoa.minimize(arg -> cost(img, shape, siz,
                                      arg[1:2:end], arg[2:2:end]),
                          x0y0, rho...)
    # FIXME: check status
    xy = rep[2]
    return (xy[1:2:end],
            xy[2:2:end])
end

#------------------------------------------------------------------------------
# A Second possibility is to fit each spot individually.
#
# This is much faster:
#   - memoryless model: 2.769 ms (5088 allocations: 300.84 KiB)
#   - workspace model:  2.569 ms (5088 allocations: 300.84 KiB)
#   - workspace model:  1.922 ms (4485 allocations: 278.86 KiB) ???
#   - workspace model + Newuoa.minimize workspace:
#                       1.918 ms (4399 allocations: 172.83 KiB)
#   - workspace model + Newuoa.minimize! workspace:
#                       1.930 ms (4311 allocations: 164.58 KiB)
#   - workspace model + Newuoa.newuoa! workspace:
#                       1.917 ms (4223 allocations: 157.70 KiB)
# against
#   - memoryless model: 1.279 s (5758 allocations: 3.75 MiB).
#   - workspace model:  1.241 s (5758 allocations: 3.75 MiB)
#
# Speedup: 462


"""
   Centroiding.fit([wgt,] img, x0, y0) -> (x, y)

yeilds the coordnates `(x,y)` of the nearest spots found in image `img`
starting at positions `(x0,y0)`.  Optional argument `wgt` can be used to
specify the repective weights for the pixels of `img`.

Mike Powell's `NEWUOA` algorithm is used to carry the optimization.

In addition to the keywords of `newuoa!`, the following keywords are allowed:

* Keyword `shape` specifies the shape model to fit (a separable cubic B-spline
  by default).

* Keyword `siz` specifies the *size* of the spots (in pixel units).

* Keyword `work` can be specified with a workspace vector (of `Cdouble`) which
  is passed to the optimizer and resized as needed.  This is useful to reduce
  allocations (and garbage collections).

* Keyword `rho` is a tuple which specifies the initial and final radius of the
  trust region (in pixel units).

* Keyword `quiet` can be set `true` to not print warning messages.

When the optimization algorithm makes too many iterations, a warning message is
printed.  Other errors result in throwing an exception.

"""
function fit(wgt::AbstractMatrix{<:AbstractFloat},
             img::AbstractMatrix{<:AbstractFloat},
             x0::Real, y0::Real; kwds...) :: NTuple{2,Cdouble}
    xy = Vector{Cdouble}(undef, 2)
    xy[1], xy[2] = x0, y0
    fit!(wgt, img, xy; kwds...)
    return (xy[1], xy[2])
end

function fit(img::AbstractMatrix{<:AbstractFloat},
             x0::Real, y0::Real; kwds...) :: NTuple{2,Cdouble}
    xy = Vector{Cdouble}(undef, 2)
    xy[1], xy[2] = x0, y0
    fit!(img, xy; kwds...)
    return (xy[1], xy[2])
end

function fit(wgt::AbstractMatrix{<:AbstractFloat},
             img::AbstractMatrix{<:AbstractFloat},
             x0::AbstractVector{<:Real},
             y0::AbstractVector{<:Real};
             kwds...)
    return fit!(wgt, img,
                copyto!(Vector{Cdouble}(undef, length(x0)), x0),
                copyto!(Vector{Cdouble}(undef, length(y0)), y0);
                kwds...)
end

function fit(img::AbstractMatrix{<:AbstractFloat},
             x0::AbstractVector{<:Real},
             y0::AbstractVector{<:Real};
             kwds...)
    return fit!(img,
                copyto!(Vector{Cdouble}(undef, length(x0)), x0),
                copyto!(Vector{Cdouble}(undef, length(y0)), y0);
                kwds...)
end

"""
```julia
Centroiding.fit!([wgt,] img, x, y) -> (x, y)
```

overwrites `x` and `y` with the coordinates of the nearest spots found in image
`img`.  Optional argument `wgt` can be used to specify the repective weights
for the pixels of `img`.  For each index `i ∈ 1:n` (where `n` is the number of
points), `(x[i],y[i])` is replaced by the coordinates of the spot found in
image `img` starting at `(x[i],y[i])`.

The coordinates may be packed in a single array:

```julia
Centroiding.fit!([wgt,] img, xy) -> xy
```

where `xy` can be a vector of 2 elements to search for a single spot or a
matrix of dimensions `(2,n)` to searcj `n` spots.  On entry, `xy` contains the
initial coordinates of the spot(s) to search, on return `xy` contains the final
coordinates of the spot(s).

For this in-place version of [`Centroiding.fit`](@ref), the elements of `x`,
`y` and `xy` must be of type `Cdouble`.

See [`Centroiding.fit`](@ref) for allowed keywords.

"""
function fit!(wgt::AbstractMatrix{<:AbstractFloat},
              img::AbstractMatrix{<:AbstractFloat},
              xy::DenseVector{Cdouble};
              shape::ShapeModel = cubicsplinemodel,
              siz::Real = 2.5,
              rho::Tuple{Real,Real} = (1.5*siz, 0.01),
              work::Vector{Cdouble} = Cdouble[],
              quiet::Bool = false,
              kwds...)
    @assert length(xy) == 2
    _siz = Cdouble(siz)
    rep = Newuoa.newuoa!(arg -> cost(wgt, img, shape, _siz, arg[1], arg[2]),
                         xy, rho...; work = work, kwds...)
    if (status = rep[1]) != Newuoa.SUCCESS
        if status == Newuoa.TOO_MANY_EVALUATIONS
            quiet || @warn "Too many evaluations in NEWUOA"
        else
            error(string("`newuoa!` failed: ",
                         Newuoa.getreason(status)))
        end
    end
    return xy
end

function fit!(img::AbstractMatrix{<:AbstractFloat},
              xy::DenseVector{Cdouble};
              shape::ShapeModel = cubicsplinemodel,
              siz::Real = 2.5,
              rho::Tuple{Real,Real} = (1.5*siz, 0.01),
              work::Vector{Cdouble} = Cdouble[],
              quiet::Bool = false,
              kwds...)
    @assert length(xy) == 2
    _siz = Cdouble(siz)
    rep = Newuoa.newuoa!(arg -> cost(img, shape, _siz, arg[1], arg[2]),
                         xy, rho...; work = work, kwds...)
    if (status = rep[1]) != Newuoa.SUCCESS
        if status == Newuoa.TOO_MANY_EVALUATIONS
            quiet || @warn "Too many evaluations in NEWUOA"
        else
            error(string("`newuoa!` failed: ",
                         Newuoa.getreason(status)))
        end
    end
    return xy
end

function fit!(img::AbstractMatrix{<:AbstractFloat},
              x::AbstractVector{Cdouble},
              y::AbstractVector{Cdouble};
              work::Vector{Cdouble} = Cdouble[],
              kwds...)
    # Checks and initializatons.
    len = length(x)
    length(y) == len ||
        throw(DimensionMismatch("`x` and `y` must have the same length"))
    xy = Vector{Cdouble}(undef, 2)

    # Loop over all nodes.
    for i in 1:len
        # FIXME: use @thread
        xy[1] = x[i]
        xy[2] = y[i]
        fit!(img, xy; work=work, kwds...)
        x[i], y[i] = xy
    end
    return (x, y)
end

function fit!(wgt::AbstractMatrix{<:AbstractFloat},
              img::AbstractMatrix{<:AbstractFloat},
              x::AbstractVector{Cdouble},
              y::AbstractVector{Cdouble};
              shape::ShapeModel = cubicsplinemodel,
              siz::Real = 2.5,
              rho::Tuple{Real,Real} = (siz, 0.01),
              work::Vector{Cdouble} = Cdouble[],
              quiet::Bool = false, kwds...)
    # Checks and initializatons.
    len = length(x)
    size(wgt) == size(img)  ||
        throw(DimensionMismatch("`wgt` and `img` must have the same size"))
    length(y) == len ||
        throw(DimensionMismatch("`x` and `y` must have the same length"))
    xy = Vector{Cdouble}(undef, 2)

    # Loop over all nodes.
    for i in 1:len
        # FIXME: use @thread
        xy[1] = x[i]
        xy[2] = y[i]
        fit!(wgt, img, xy; work=work, kwds...)
        x[i], y[i] = xy
    end
    return (x, y)
end

#------------------------------------------------------------------------------

"""
```julia
_getbbox(dims, sup, x, y) -> imin, imax, jmin, jmax
```

yields the (inclusive) bounding-box of the indices in the intersection of the
valid indices for an array of dimension `dims` with a square of size `sup`
centered at position `(x,y)`.

"""
function _getbbox(dims::NTuple{2,Int}, sup::Real, x::Real, y::Real)
    rad = Cdouble(sup/2) # radius of ROI
    imin = max( ceil(Int, x - rad), 1)
    imax = min(floor(Int, x + rad), dims[1])
    jmin = max( ceil(Int, y - rad), 1)
    jmax = min(floor(Int, y + rad), dims[2])
    return imin, imax, jmin, jmax
end

end # module
