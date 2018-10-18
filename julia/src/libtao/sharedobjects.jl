#
# sharedobjects.jl --
#
# Management of shared objects for Julia interface to the TAO C-library.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018, Éric Thiébaut.
#

function _fix_shared_object_type(type::Integer) :: Cint
    return _fix_shared_object_type(convert(Cint, type))
end

function _fix_shared_object_type(type::Cint) :: Cint
    return (0 ≤ type ≤ 255 ? (SHARED_MAGIC | type) : type)
end

function attach(::Type{SharedObject}, ident::Integer,
                type::Integer = SHARED_ANY)
    # Attach the shared object to the address space of the caller, then wrap it
    # in a Julia object.
    errs = Errors()
    ptr = ccall((:tao_attach_shared_object, taolib), Ptr{Cvoid},
                (Ref{Errors}, Cint, Cint), errs, ident,
                _fix_shared_object_type(type))
    _check(ptr != C_NULL, errs)
    return _wrap(SharedObject(), ptr)
end

function create(::Type{SharedObject}, type::Integer, size::Integer;
                perms::Integer = 0o600)
    errs = Errors()
    ptr = ccall((:tao_create_shared_object, taolib), Ptr{Cvoid},
                (Ref{Errors}, Cint, Csize_t, Cuint), errs,
                _fix_shared_object_type(type), size, perms)
    _check(ptr != C_NULL, errs)
    return _wrap(SharedObject(), ptr)
end

function detach(obj::AnySharedObject)
    if (ptr = obj.ptr) != C_NULL
        obj.ptr = C_NULL # to avoid detaching more than once
        _check(_detach(ptr))
    end
    return nothing
end

# This method assumes that the given pointer is correct and that the caller
# will deal with the errors if any.
function _detach(ptr::Ptr{Cvoid})
    errs = Errors()
    ccall((:tao_detach_shared_object, taolib), Cint,
          (Ref{Errors}, Ptr{Cvoid}), errs, ptr)
    return errs
end

# Wrap a shared object attached at address `ptr` into a Julia object `obj`.
function _wrap(obj::AnySharedObject, ptr::Ptr{Cvoid})
    obj.ptr = ptr
    return finalizer(_destroy, obj)
end

# Method `_destroy` called to finalize an object is very similar to `detach`
# except that it does not throws errors but print them instead.
function _destroy(obj::AnySharedObject)
    if (ptr = obj.ptr) != C_NULL
        obj.ptr = C_NULL # to avoid detaching more than once
        errs = _detach(ptr)
        any_errors(errs) && report_errors(errs)
    end
end

# Extend `unsafe_convert` to return the correct pointer when a shared object is
# passed to `ccall`.  This could also be the opportunity to check for the
# validity of the pointer but it turns out that it was faster (and simpler) to
# have it in the C library.  Note that the `unsafe_convert` method is used by
# `ccall`, it is said to be *unsafe* because there is no guaranties that the
# returned address will remain valid.
Base.unsafe_convert(::Type{Ptr{Cvoid}}, obj::AnySharedObject) = obj.ptr

function lock(obj::AnySharedObject)
    errs = Errors()
    code = ccall((:tao_lock_shared_object, taolib), Cint,
                 (Ref{Errors}, Ptr{Cvoid},), errs, obj)
    _check(code != -1, errs)
end

function trylock(obj::AnySharedObject)
    errs = Errors()
    code = ccall((:tao_try_lock_shared_object, taolib), Cint,
                 (Ref{Errors}, Ptr{Cvoid},), errs, obj)
    _check(code != -1, errs)
    return (code != 0)
end

function unlock(obj::AnySharedObject)
    errs = Errors()
    code = ccall((:tao_unlock_shared_object, taolib), Cint,
                 (Ref{Errors}, Ptr{Cvoid},), errs, obj)
    _check(code != -1, errs)
end

get_type(obj::AnySharedObject) =
    ccall((:tao_get_shared_object_type, taolib), Cint, (Ptr{Cvoid},), obj)

get_size(obj::AnySharedObject) =
    ccall((:tao_get_shared_object_size, taolib), Csize_t, (Ptr{Cvoid},), obj)

get_ident(obj::AnySharedObject) =
    ccall((:tao_get_shared_object_ident, taolib), Cint, (Ptr{Cvoid},), obj)
