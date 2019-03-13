#
# RandomWavefronts.jl --
#
# Random wavefront generators.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018-2019, Éric Thiébaut & Michel Tallon.
#

module RandomWavefronts

using LinearAlgebra

"""

The following example shows how to create a random generator to simulate a
random Kolmogorov wavefront:

```julia
using RandomWavefronts
dim = 50
gen = RandomWavefronts.Generator(:Kolmogorov, dim; sigma=19, r0=dim/10)
wav = rand(gen)
```

"""
struct Generator{T<:AbstractFloat,M<:AbstractMatrix{T}}
    L::M
    dims::NTuple{2,Int}
    function Generator{T,M}(L::M,
                            dims::NTuple{2,Int}) where {T<:AbstractFloat,
                                                        M<:AbstractMatrix{T}}
        new{T,M}(L, dims)
    end
end

function Generator(L::M, dims::NTuple{2,Int}) where {T<:AbstractFloat,
                                                     M<:AbstractMatrix{T}}
    return Generator{T,M}(L, dims)
end

Base.eltype(::Generator{T}) where {T} = T

# const Dims{N} = NTuple{N,Int}

makedims(dim::Int) = (dim,)
makedims(dim::Integer) = (Int(dim),)
makedims(dims::Integer...) = makedims(dims)
makedims(dims::Dims) = dims
makedims(dims::Tuple{Vararg{Integer}}) = map(Int, dims)

Generator(S::Union{Val,Symbol}, n1::Integer, n2::Integer=n1; kwds...) =
    Generator(S, (n1, n2); kwds...)

Generator(S::Union{Val,Symbol}, dims::NTuple{2,Integer}; kwds...) =
    Generator(S, makedims(dims); kwds...)

function Generator(S::Union{Val,Symbol}, dims::NTuple{2,Int}; kwds...)
    n = prod(dims)
    L = cholesky!(reshape(covar(S, dims...; kwds...), n, n)).L
    return Generator(L, dims)
end

Base.rand(gen::Generator{T}) where {T<:AbstractFloat} =
    reshape(gen.L*randn(T, prod(gen.dims)), gen.dims)

"""

```julia
RandomWavefronts.structfn(S, dims; kwds...)
```

yields the structure function of a wavefront of size `dims` whose distribution
is given by `S`, for instance:

```julia
dim = 70
SFn = RandomWavefronts.structfn(:Kolmogorov, dim; r0=dim/10)
```

# Description

The structure function of the wavefront is the expected value of the quadratic
difference between two phases of a turbulent wavefront:

    S[i,j] = ⟨(w[i] - w[j])²⟩ = f(‖r[i] - r[j]‖)

where, e.g.:

    f(r) = 6.88 (r/r0)^(5/3)

for a turbulent wavefront obeying Kolmogorov’s law and with `r0` the Fried's
parameter.

This structure function is stationary (shift-invariant) and isotropic since it
only depends on the distance `‖r[i] - r[j]‖` between the considered positions
`r[i]` and `r[j]` in the wavefront.  From the structure function, can be
deduced the spatial covariance of the wavefront between two positions in the
pupil:

    C[i,j] = ⟨w[i]⋅w[j]⟩ = (σ²[i] + σ²[j] - S[i,j])/2

where `σ²[i] ≡ ⟨w[i]²⟩` is the variance of the wavefront at position `r[i]`.

"""
function structfn(::Val{:Kolmogorov},
                  dims::Dims{2};
                  r0::Real = 1.0,
                  flatten::Bool=false)
    T = Float64
    n1, n2 = dims
    S = Array{T}(undef, n1, n2, n1, n2)
    @inbounds for j2 in 1:n2, j1 in 1:n1
        for i2 in 1:n2
            @simd for i1 in 1:n1
                r = hypot(i1 - j1, i2 - j2)
                S[i1,i2,j1,j2] = 6.88*(r/r0)^(5/3)
            end
        end
    end
    return (flatten ? reshape(S, n1*n2, n1*n2) : S)
end

function defaultsigma(::Val{:Kolmogorov}, rhomax::Real)
    return sqrt(3.44*(rhomax)^(5/3))
end

"""

```julia
RandomWavefronts.covar(S, dims; kwds...)
```

yields the covariance of a wavefront of size `dims` whose distribution
is given by `S`, for instance:

```julia
dim = 70
C = RandomWavefronts.covar(:Kolmogorov, dim; r0=dim/10, sigma=19)
```

yields the covariance of a wavefront phase of size `(dim,dim)` where keyword
`r0` specifies Fried's parameter (in pixel units) and keyword `sigma` specifues
the uniform standard deviation of the wavefront phase.

See [`RandomWavefronts.structfn`](@ref) for more explanations.

"""
function covar(K::Val{:Kolmogorov},
               dims::Dims{2};
               r0::Real = 1.0,
               sigma::Real = defaultsigma(K, hypot(dims[1] -1, dims[2] - 1)/r0),
               flatten::Bool=false)
    n1, n2 = dims
    T = Float64
    C = Array{T}(undef, n1, n2, n1, n2)
    v = T(sigma)^2
    @inbounds for j2 in 1:n2, j1 in 1:n1
        for i2 in 1:n2
            @simd for i1 in 1:n1
                r = hypot(i1 - j1, i2 - j2)
                C[i1,i2,j1,j2] = v - 3.44*(r/r0)^(5/3)
            end
        end
    end
    return (flatten ? reshape(C, n1*n2, n1*n2) : C)
end


# Extend methods to automatically convert arguments.

structfn(S::Symbol, args...; kwds...) =
    structfn(Val(S), args...; kwds...)

structfn(S::Val, n1::Integer, n2::Integer=n1; kwds...) =
    structfn(S, makedims(n1, n2); kwds...)

structfn(S::Val, dims::NTuple{2,Integer}; kwds...) =
    structfn(S, makedims(dims); kwds...)

covar(S::Symbol, args...; kwds...) =
    covar(Val(S), args...; kwds...)

covar(S::Val, n1::Integer, n2::Integer=n1; kwds...) =
    covar(S, makedims(n1, n2); kwds...)

covar(S::Val, dims::NTuple{2,Integer}; kwds...) =
    covar(S, makedims(dims); kwds...)

end # module
