#
# LibTAO.jl --
#
# Provide Julia interface to the C-library TAO, a Toolkit for Adaptive Optics.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018, Éric Thiébaut.
#

module LibTAO

# Extends some methods in parent TAO module.
import ...TAO: AbstractSharedObject, AnySharedObject, SharedObject, SharedArray, TimeStamp,
    SHARED_ANY, SHARED_MAGIC, SHARED_OBJECT, SHARED_ARRAY, SHARED_CAMERA,
    SHARED_ARRAY_MAX_NDIMS, SHARED_ARRAY_TYPES, SharedArrayTypes, SHARED_ARRAY_ELTYPES,
    create, attach, detach, lock, trylock, unlock,
    get_type, get_ident, get_size

# Constant `taolib` is defined in another file which is built at installation
# time.
include("../../deps/deps.jl")

include("types.jl")
include("errors.jl")
include("sharedobjects.jl")
include("sharedarrays.jl")

# FIXME: dangerous!!!
function _push_error(errs::Errors, func::String, code::Integer)
    ccall((:tao_push_error, taolib), Cvoid,
          (Ref{Errors}, Cstring, Cint), errs, func, code)
end

# FIXME: extend show method for TaoError and ErrorInfo



end # module
