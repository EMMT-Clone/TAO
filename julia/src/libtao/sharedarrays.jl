#
# sharedarrays.jl --
#
# Management of shared multi-dimensional arrays for Julia interface to TAO
# C-library.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018, Éric Thiébaut.
#

# Make a `TAO.SharedArray{T,N}` behaves like an array.  Note that, for
# performance reasons, reading/writing the array contents is done without any
# attempt to lock the object before.  Locking has to be done appropriately
# when such an object is used.
@inline Base.eltype(obj::SharedArray{T,N}) where {T,N} = T
@inline Base.length(obj::SharedArray) = length(obj.arr)
@inline Base.ndims(obj::SharedArray{T,N}) where {T,N} = N
@inline Base.size(obj::SharedArray) = size(obj.arr)
@inline Base.size(obj::SharedArray, d) = size(obj.arr, d)
@inline Base.axes(obj::SharedArray) = axes(obj.arr)
@inline Base.axes(obj::SharedArray, d) = axes(obj.arr, d)
@inline Base.eachindex(obj::SharedArray) = eachindex(obj.arr)
@inline Base.stride(obj::SharedArray, d) = stride(obj.arr, d)
@inline Base.strides(obj::SharedArray) = strides(obj.arr)

# FIXME: similar
@inline Base.similar(obj::SharedArray, args...) = similar(obj.arr, args...)
@inline Base.reshape(obj::SharedArray, dims...) = reshape(obj.arr, dims...)
@inline Base.copy(obj::SharedArray) = copy(obj.arr)
@inline Base.deepcopy(obj::SharedArray) = deepcopy(obj.arr)
@inline Base.fill!(obj::SharedArray, val) = fill!(obj.arr, val)

@inline Base.getindex(obj::SharedArray, inds...) =
    getindex(obj.arr, inds...)
@inline Base.setindex!(obj::SharedArray, val, inds...) =
    setindex!(obj.arr, val, inds...)

Base.IndexStyle(::Type{<:SharedArray}) = IndexLinear()

# Make a `TAO.SharedArray{T,N}` iterable.
Base.iterate(obj::SharedArray) = iterate(obj.arr)
Base.iterate(obj::SharedArray, state) = iterate(obj.arr, state)

# Extend `unsafe_convert` to check for the validity of the pointer when
# a shared array is passed to `ccall`.
Base.unsafe_convert(::Type{Ptr{T}}, obj::SharedArray{T,N}) where {T,N} =
    Ptr{T}(unsafe_convert(Ptr{Cvoid}, obj))
function Base.unsafe_convert(::Type{Ptr{Cvoid}},
                             obj::SharedArray{T,N}) where {T,N}
    obj.ptr != C_NULL || @error "Detached shared array"
    return obj.ptr
end

function create(::Type{SharedArray{T}}, dims::Integer...; kwds...) where {T}
    N = length(dims)
    return create(SharedArray{T,N}, convert(NTuple{N,Int}, dims); kwds...)
end

function create(::Type{SharedArray{T,N}},
                dims::NTuple{N,Int};
                perms::Integer = 0o600) where {T,N}
    @assert haskey(SHARED_ARRAY_ELTYPES, T) "Invalid array element type"
    eltype = SHARED_ARRAY_ELTYPES[T]
    errs = Errors()
    ptr = ccall((:tao_create_shared_array, taolib), Ptr{Cvoid},
                (Ref{Errors}, Cint, Cint, Ptr{Csize_t}, Cuint),
                errs, eltype, N, Csize_t[dims...], perms)
    _check(ptr != C_NULL, errs)
    return _wrap(SharedArray{T,N}, ptr, dims)
end

function attach(::Type{SharedArray}, ident::Integer)
    errs = Errors()
    ptr = ccall((:tao_attach_shared_array, taolib), Ptr{Cvoid},
                (Ref{Errors}, Cint), errs, ident)
    _check(ptr != C_NULL, errs)
    eltype = ccall((:tao_get_array_eltype, taolib), Cint,
                   (Ptr{Cvoid},), ptr)
    1 ≤ eltype ≤ length(SHARED_ARRAY_TYPES) ||
        _detach_on_error(ptr, "Bad element type")
    N = convert(Int, ccall((:tao_get_array_ndims, taolib), Cint,
                           (Ptr{Cvoid},), ptr))
    1 ≤ N ≤ SHARED_ARRAY_MAX_NDIMS ||
        _detach_on_error(ptr, "Bad number of dimensions")
    dims = Array{Int}(undef, N)
    for d in 1:N
        dims[d] = ccall((:tao_get_array_size, taolib), Csize_t,
                        (Ptr{Cvoid}, Cint), ptr, d)
        dims[d] ≥ 1 || _detach_on_error(ptr, "Bad dimension $d")
    end
    T = SHARED_ARRAY_TYPES[eltype]
    return _wrap(SharedArray{T,N}, ptr, (dims...,))
end

function detach(obj::SharedArray{T,N}) where {T,N}
    if obj.ptr != C_NULL
        # Detaching a TAO shared array makes the associated Julia array invalid
        # so we replace it with an array of the coorect type but with all
        # dimensions equal to zero.
        ptr = obj.ptr
        obj.arr = Array{T,N}(undef, ntuple(i -> 0, N))
        obj.ptr = C_NULL
        _check(_detach(ptr))
    end
end

function _detach_on_error(ptr::Ptr{Cvoid}, msg::AbstractString)
    errs = detach(ptr::Ptr{Cvoid})
    any_errors(errs) && discard_errors(errs)
    @error msg
end

function _wrap(::Type{SharedArray{T,N}}, ptr::Ptr{Cvoid},
               dims::NTuple{N,Int}) where {T,N}
    data = ccall((:tao_get_array_data, taolib), Ptr{T}, (Ptr{Cvoid},), ptr)
    arr = unsafe_wrap(Array, data, dims; own = false)
    obj = SharedArray{T,N}(ptr, arr)
    return finalizer(_destroy, obj)
end

get_nreaders(obj::SharedArray) =
    ccall((:tao_get_array_nreaders, taolib), Cint,
          (Ptr{Cvoid},), obj)

adjust_nreaders!(obj::SharedArray, adj::Integer) =
    ccall((:tao_adjust_array_nreaders, taolib), Cint,
          (Ptr{Cvoid}, Cint), obj, adj)

get_nwriters(obj::SharedArray) =
    ccall((:tao_get_array_nwriters, taolib), Cint,
          (Ptr{Cvoid},), obj)

adjust_nwriters!(obj::SharedArray, adj::Integer) =
    ccall((:tao_adjust_array_nwriters, taolib), Cint,
          (Ptr{Cvoid}, Cint), obj, adj)

get_counter(obj::SharedArray) =
    ccall((:tao_get_array_counter, taolib), Int64,
          (Ptr{Cvoid},), obj)

set_counter!(obj::SharedArray, cnt::Integer) =
    ccall((:tao_set_array_counter, taolib), Cvoid,
          (Ptr{Cvoid}, Int64), obj, cnt)

function get_timestamp(obj::SharedArray)
    ts_sec = Ref{Int64}()
    ts_nsec = Ref{Int64}()
    ccall((:tao_get_array_timestamp, taolib), Cvoid,
          (Ptr{Cvoid}, Ptr{Int64}, Ptr{Int64}),
          obj, ts_sec, ts_nsec)
    return TimeStamp(ts_sec[], ts_nsec[])
end

set_timestamp!(obj::SharedArray, secs::Real) =
    set_timestamp!(obj, TimeStamp(secs))

set_timestamp!(obj::SharedArray, ts::TimeStamp) =
    ccall((:tao_set_array_timestamp, taolib), Cvoid,
          (Ptr{Cvoid}, Int64, Int64), obj, ts.sec, ts.nsec)
