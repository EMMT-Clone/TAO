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

using TiPi

export
    #TaoError,
    attach,
    create,
    detach,
    lock,
    trylock,
    unlock

include("tao/constants.jl")
include("tao/types.jl")
include("tao/clib.jl")
include("tao/utils.jl")
include("tao/weighted.jl")
include("tao/detector.jl")
include("tao/kolmogorov.jl")

end # module
