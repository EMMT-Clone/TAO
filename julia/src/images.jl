#
# images.jl --
#
# Image (i.e. 2D arrays) processing for Julia package TAO (a Toolkit for
# Adaptive Optics software).
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018, Éric Thiébaut.
#

module Images

export
    boxfilter,
    boxfilter!,
    sobel,
    sobel!

"""

```julia
boxfilter([T,] f, A) -> B
```

yields the result of applying a filter in a 3×3 box around all elements of the
2D array `A`.  The result `B` is a 2D array.  Optional argument `T` may be used
to specify the type of the elements of the result.  The filter `f` can be:

* A function which takes 9 arguments and is used as:

  ```julia
  B[x,y] = f(A[x-1,y-1], A[x,y-1], A[x+1,y-1],
             A[x-1,y],   A[x,y],   A[x+1,y],
             A[x-1,y+1], A[x,y+1], A[x+1,y+1])
  ```

* The coefficients of a 3×3 kernel specified as a 3×3 tuple (whose elements
  have the same type) or a 3×3 array.  The coefficients of the kernel are used
  as:

  ```julia
  B[x,y] = ((f[1,1]*A[x-1,y-1] + f[2,1]*A[x,y-1] + f[3,1]*A[x+1,y-1]) +
            (f[1,2]*A[x-1,y]   + f[2,2]*A[x,y]   + f[3,2]*A[x+1,y]  ) +
            (f[1,3]*A[x-1,y+1] + f[2,3]*A[x,y+1] + f[3,3]*A[x+1,y+1]))
  ```

The in-place version:

```julia
boxfilter!(B, f, A) -> B
```

overwrites the contents of `B` with the result of the operation.

See also: [`sobel`](@ref).

"""
function boxfilter(f::Function,
                   A::AbstractMatrix{T}) where {T}
    len = length(A)
    len ≥ 1 || throw(ArgumentError("array must have at least one element"))
    a1 = A[1,1]
    f1 = f(a1, a1, a1,
           a1, a1, a1,
           a1, a1, a1)
    R = typeof(f1)
    if len == 1
        dst = Array{R}(undef, 1, 1)
        dst[1,1] = f1
        return dst
    end
    return boxfilter(R, f, A)
end

boxfilter(::Type{R}, f::Function, A::AbstractMatrix{T}) where {R,T} =
    boxfilter!(Array{R}(undef, size(A)), f, A)

function boxfilter!(dst::AbstractMatrix{R},
                    f::Function,
                    A::AbstractMatrix{T}) where {R,T}
    size(dst) == size(A) || throw(DimensionMismatch("source and destination must have the same dimensions"))
    width, height = size(A)
    if height ≥ 1 && width ≥ 1
        @inbounds for y in 1:height
            ym1 = max(y - 1, 1)
            yp1 = min(y + 1, height)
            a2 = a3 = A[1, ym1]
            a5 = a6 = A[1, y  ]
            a8 = a9 = A[1, yp1]
            @simd for x in 1:width
                xp1 = min(x + 1, width)
                a1, a2, a3 = a2, a3, A[xp1, ym1]
                a4, a5, a6 = a5, a6, A[xp1, y  ]
                a7, a8, a9 = a8, a9, A[xp1, yp1]
                dst[x,y] = f(a1, a2, a3,
                             a4, a5, a6,
                             a7, a8, a9)
            end
        end
    end
    return dst
end

boxfilter(ker::AbstractMatrix{K}, A::AbstractMatrix{T}) where {K,T} =
    boxfilter(_repack_box_kernel(ker), A)

boxfilter(ker::NTuple{3,NTuple{3,K}}, A::AbstractMatrix{T}) where {K,T} =
    boxfilter(promote_type(K,T), ker, A)

function boxfilter(::Type{R}, ker::AbstractMatrix{K},
                   A::AbstractMatrix{T}) where {R,K,T}
    return boxfilter(R, _repack_box_kernel(ker), A)
end

function boxfilter(::Type{R}, ker::NTuple{3,NTuple{3,K}},
                   A::AbstractMatrix{T}) where {R,K,T}
     return boxfilter!(Array{R}(undef, size(A)), _repack_box_kernel(ker), A)
end

function boxfilter!(dst::AbstractMatrix{R},
                    ker::NTuple{3,NTuple{3,K}},
                    A::AbstractMatrix{T}) where {R,K,T}
    size(dst) == size(A) || throw(DimensionMismatch("source and destination must have the same dimensions"))
    width, height = size(A)
    if height ≥ 1 && width ≥ 1
        w1, w2, w3 = ker[1]
        w4, w5, w6 = ker[2]
        w7, w8, w9 = ker[3]
        @inbounds for y in 1:height
            ym1 = max(y - 1, 1)
            yp1 = min(y + 1, height)
            a2 = a3 = A[1, ym1]
            a5 = a6 = A[1, y  ]
            a8 = a9 = A[1, yp1]
            @simd for x in 1:width
                xp1 = min(x + 1, width)
                a1, a2, a3 = a2, a3, A[xp1, ym1]
                a4, a5, a6 = a5, a6, A[xp1, y  ]
                a7, a8, a9 = a8, a9, A[xp1, yp1]
                dst[x,y] = ((w1*a1 + w2*a2 + w3*a3) +
                            (w4*a4 + w5*a5 + w6*a6) +
                            (w7*a7 + w8*a8 + w9*a9))
            end
        end
    end
    return dst
end

@doc @doc(boxfilter) boxfilter!

function _repack_box_kernel(ker::AbstractMatrix{K}) where {K}
    size(ker) == (3,3) || throw(DimensionMismatch("expecting a 3×3 kernel array"))
    return ((ker[1,1], ker[2,1], ker[3,1]),
            (ker[1,2], ker[2,2], ker[3,2]),
            (ker[1,3], ker[2,3], ker[3,3]))
end

"""

```julia
sobel(img) -> grd
```

yields the spatial gradients of image `img` approximated by Sobel's filter.
The result is a 2-by-`size(img)` array such that:

```julia
grd[1,...] = ∂img/∂x
grd[2,...] = ∂img/∂y
```

where `x` and `y` are assumed to be the coordinates along the 1st and 2nd
dimensions of the image.

See also: [`boxfilter`](@ref).

"""
sobel(A::AbstractMatrix{T}) where {T<:Real} =
    sobel(float(T), A)

sobel(::Type{R}, A::AbstractMatrix{T}) where {R,T} =
    sobel!(Array{R}(undef, 2, size(A)...), A)

function sobel!(dst::AbstractArray{R,3}, A::AbstractMatrix{T}) where {R,T}
    dims = size(dst)
    dims[1] == 2 || throw(DimensionMismatch("leading dimension of gradient must be 2"))
    dims[2:3] == size(A) || throw(DimensionMismatch("trailing dimensions of gradient must match image dimensions"))
    width, height = size(A)
    if height ≥ 1 && width ≥ 1
        q = R(1)/R(8)
        local a7::R, a8::R, a9::R
        local a4::R, a5::R, a6::R
        local a1::R, a2::R, a3::R
        @inbounds for y in 1:height
            ym1 = max(y - 1, 1)
            yp1 = min(y + 1, height)
            a2 = a3 = A[1, ym1]
            a5 = a6 = A[1, y  ]
            a8 = a9 = A[1, yp1]
            @simd for x in 1:width
                xp1 = min(x + 1, width)
                a1, a2, a3 = a2, a3, A[xp1, ym1]
                a4, a5, a6 = a5, a6, A[xp1, y  ]
                a7, a8, a9 = a8, a9, A[xp1, yp1]
                dst[1,x,y] = q*((a3 + 2*a6 + a9) -
                                (a1 + 2*a4 + a7))
                dst[2,x,y] = q*((a7 + 2*a8 + a9) -
                                (a1 + 2*a2 + a3))
            end
        end
    end
    return dst
end

end # module
