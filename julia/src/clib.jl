#
# clib.jl --
#
# Deal with the loading of the interface to TAO C-library and provide fallback
# methods for shared objects and inter-process communication provided by this
# library.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018, Éric Thiébaut.
#

# The following methods are fallbacks which capture any calls to the method
# either because the C library is not loaded or because arguments are invalid.
# The documentation is however provided here.

"""
```julia
create(TAO.SharedArray{T}, dims...; perms=0o600) -> arr
```

cretates a new shared TAO array with element type `T` and dimensions `dims` and
returns an instance attached to it.  Keyword `perms` can be used to grant
access permissions other than having read and write permissions for the creator
only.

```julia
create(TAO.SharedObject, type, size; perms=0o600) -> obj
```

creates a new shared TAO object of given type and size (in bytes) and returns
an instance attached to it.  Keyword `perms` has the same meaning as above.


"""
create(::Type{T}, args...; kwds...) where {T<:AnySharedObject} =
    _clib_error("create", T)

"""

```julia
attach(TAO.SharedObject, ident, type=TAO.SHARED_ANY) -> obj
```

attaches the shared TAO object identified by `ident` to the data space of the
caller and returns a new instance of `TAO.SharedObject` associated with it.
Argument `type` can be used to restrict to a specific type of shared object
(throwing an exception if `ident` does not correspond to an object of that
type).

```julia
attach(TAO.SharedArray, ident) -> arr
```

attaches the shared TAO array identified by `ident` to the data space of the
caller and returns a new instance of `TAO.SharedArray` associated with it and
which can be used as any Julia `DenseArray`.

"""
attach(::Type{T}, args...; kwds...) where {T<:AnySharedObject} =
    _clib_error("attach", T)

"""
```julia
detach(obj::TAO.AnySharedObject)
```

detaches TAO shared-object `obj` from the data space of the caller.

"""
detach(obj::AnySharedObject, args...; kwds...) =
    _clib_error("detach", obj)

"""
```julia
lock(obj::TAO.AnySharedObject)
```

locks TAO shared-object `obj`.

"""
lock(obj::AnySharedObject, args...; kwds...) =
    _clib_error("lock", obj)

"""
```julia
unlock(obj::TAO.AnySharedObject)
```

unlocks TAO shared-object `obj`.

"""
unlock(obj::AnySharedObject, args...; kwds...) =
    _clib_error("unlock", obj)

"""
```julia
trylock(obj::TAO.AnySharedObject) -> boolean
```

tries to lock TAO shared-object `obj`.

"""
trylock(obj::AnySharedObject, args...; kwds...) =
    _clib_error("trylock", obj)

"""
```julia
TAO.get_type(obj::TAO.AnySharedObject)
```

yields the type identifier of TAO shared-object `obj`.

"""
get_type(obj::AnySharedObject, args...; kwds...) =
    _clib_error("get_type", obj)

"""
```julia
TAO.get_size(obj::TAO.AnySharedObject)
```

yields the full-size of TAO shared-object `obj`.

"""
get_size(obj::AnySharedObject, args...; kwds...) =
    _clib_error("get_size", obj)

"""
```julia
TAO.get_ident(obj::TAO.AnySharedObject)
```

yields the unique identifier of TAO shared-object `obj`.

"""
get_ident(obj::AnySharedObject, args...; kwds...) =
    _clib_error("get_ident", obj)


function _clib_error(func::String, arg)
    if !_clib_loaded[]
        @error "TAO C-library is not yet loaded, call `TAO.loadclib()` before"
    else
        str = isa(arg, DataType) ? "$arg" : "$(typeof(arg))"
        throw(ArgumentError("invalid arguments in TAO.$func($str, ...)"))
    end
end

const _clib_loaded = Ref{Bool}(false)
const _clib_source = joinpath(@__DIR__, "libtao", "LibTAO.jl")

"""

`TAO.loadclib()` loads the TAO C library and its interface.  This function must
be called prior to using shared objects and inter-process communication
provided by TAO.

"""
function loadclib()
    if !_clib_loaded[]
       @eval include(_clib_source)
        _clib_loaded[] = true
    end
    return nothing
end
