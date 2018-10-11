#
# TAO.jl --
#
# Julia package TAO, a Toolkit for Adaptive Optics.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018, Éric Thiébaut.
#

isdefined(Base, :__precompile__) && __precompile__(true)

module TAO

import Base: detach, lock, trylock, unlock

export
    #TaoError,
    attach,
    create,
    detach,
    lock,
    trylock,
    unlock

include("constants.jl")
include("types.jl")
include("clib.jl")
include("utils.jl")
include("weighted.jl")
include("detector.jl")
include("kolmogorov.jl")

end # module
