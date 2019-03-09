#
# fitting.jl -
#
# Fitting correction for Themis adaptive optics software.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018-2019, Éric Thiébaut and Michel Tallon.
#

"""
```julia
subsamplingmatrix([T=Float64,] msk; flat=false)
```

yields the sub-sampling matrix defined by the boolean mask `msk`.  The
sub-sampling matrix selects the components in an array of same size as the
mask `msk` and corresponding to true values in the mask.  If keyword `flat`
is true, the result is a 2D array whatever the dimensions of `msk`;
otherwise, the result is an array whose trailing dimensions are those of
the mask.

"""
subsamplingmatrix(msk::AbstractArray{Bool}; kwds...) =
    subsamplingmatrix(Float64, msk; kwds...)

function subsamplingmatrix(::Type{T},
                           msk::AbstractArray{Bool};
                           flat::Bool = false) where {T<:Real}
    m = count(msk)
    dims = (flat ? (m, length(msk)) : (m, size(msk)...))
    S = zeros(T, dims)
    i, j = 0, 0
    @inbounds for k in eachindex(msk)
        j += 1
        if msk[k]
            i += 1
            S[i,j] = one(T)
        end
    end
    return S
end

"""
```julia
finitedifferencematrix([T=Float64,] dims; order=1, flat=false)
```

yields a finite difference matrix.

"""
finitedifferencematrix(dims::NTuple{2,Int}; kwds...) =
    finitedifferencematrix(Float64, dims; kwds...)

function finitedifferencematrix(::Type{T},
                                dims::NTuple{2,Int};
                                order::Integer = 1,
                                flat::Bool = false) where {T<:Real}
    n1, n2 = dims
    D = zeros(T, 2, dims..., dims...)
    if order == 1
        @inbounds for i2 in 1:n2, i1 in 1:n1
            # Assume forward finite differences.
            if i1 < n1
                D[1,i1,i2,i1,  i2] = -one(T)
                D[1,i1,i2,i1+1,i2] = +one(T)
            end
            if i2 < n2
                D[2,i1,i2,i1,i2  ] = -one(T)
                D[2,i1,i2,i1,i2+1] = +one(T)
            end
        end
    elseif order == 2
        @inbounds for i2 in 1:n2, i1 in 1:n1
            # Assume forward finite differences.
            if 1 < i1 < n1
                D[1,i1,i2,i1-1,i2] = +T(1)
                D[1,i1,i2,i1,  i2] = -T(2)
                D[1,i1,i2,i1+1,i2] = +T(1)
            end
            if 1 < i2 < n2
                D[2,i1,i2,i1,i2-1] = +T(1)
                D[2,i1,i2,i1,i2  ] = -T(2)
                D[2,i1,i2,i1,i2+1] = +T(1)
            end
        end
    else
        error("order $order not implemented")
    end
    if flat
        n = prod(dims)
        return reshape(D, 2*n, n)
    else
        return D
    end
end

"""
```julia
matrices4map(G, msk, order=2) -> H, H'*H, D'*D
```

yields the matrices needed for solving the MAP (Maximum A Posteriori)
problem of wavefront reconstruction.  The wavefront being represented in
the same space as the actuators (except no holes).

"""
function matrices4map(G::AbstractMatrix{T},
                      msk::AbstractArray,
                      order::Integer = 2) where {T<:AbstractFloat}
    P = subsamplingmatrix(T, msk; flat=true)
    H = G*P
    if 1 ≤ order ≤ 2
        D = finitedifferencematrix(T, size(msk); order=order, flat=true)
        DtD = D'*D
    elseif order == 0
        n = length(msk)
        DtD = zeros(T, n, n)
        DtD[1:n+1:n*n] .= T(1)
    end
    return H, H'*H, DtD
end

function matrices4map(G::AbstractArray{T,3},
                      args...) where {T<:AbstractFloat}
    @assert size(G, 1) == 2
    return matrices4map(reshape(G, 2*size(G,2), size(G,3)), args...)
end

function solvemapproblem(H::AbstractMatrix,
                         HtH::AbstractMatrix,
                         DtD::AbstractMatrix,
                         mu::Real,
                         d::AbstractVector)
    A = HtH + mu*DtD
    b = H'*d
    return ldiv!(cholesky!(A), b) # same as A\b but optimized
end

function solvemapproblem(H::AbstractMatrix,
                         HtH::AbstractMatrix,
                         DtD::AbstractMatrix,
                         mu::Real,
                         d::AbstractMatrix)
    @assert size(d, 1) == 2
    return solvemapproblem(H, HtH, DtD, mu, reshape(d, length(d)))
end

function solvemapproblem(H::AbstractMatrix,
                         HtH::AbstractMatrix,
                         DtD::AbstractMatrix,
                         mu::Real,
                         dx::AbstractVector,
                         dy::AbstractVector)
    @assert length(dx) == length(dy)
    n = length(dx)
    T = promote_type(eltype(dx), eltype(dy))
    d = Vector{T}(undef, 2*n)
    @inbounds @simd for i in 1:n
        d[2i-1] = dx[i]
        d[2i]   = dy[i]
    end
    return solvemapproblem(H, HtH, DtD, mu, d)
end
