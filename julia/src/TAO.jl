#
# TAO.jl --
#
# Provide Julia interface to TAO, a Toolkit for Adaptive Optics.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO library (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018, Éric Thiébaut.
#

module TAO

import Base: detach, lock, trylock, unlock

export
    TaoError, attach, detach, lock, trylock, unlock

include("constants.jl")
include("types.jl")
include("errors.jl")
include("sharedobjects.jl")
include("sharedarrays.jl")
include("utils.jl")


# FIXME: dangerous!!!
function _push_error(errs::Errors, func::String, code::Integer)
    ccall((:tao_push_error, taolib), Cvoid,
          (Ref{Errors}, Cstring, Cint), errs, func, code)
end

# FIXME: extend show method for TaoError and ErrorInfo



end # module
