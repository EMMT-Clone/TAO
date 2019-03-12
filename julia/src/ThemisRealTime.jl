#
# ThemisRealTime.jl --
#
# Julia module Themis for Themis adaptive optics real-time software.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018-2019, Éric Thiébaut & Michel Tallon.
#

module ThemisRealTime

using Printf, LinearAlgebra, Statistics
using Phoenix, Alpao
using Phoenix: TimeoutError, Camera
using Themis
import Themis: InteractionMatrix

include("themis/realtime/loop.jl")
include("themis/realtime/calibration.jl")

end # module ThemisRealTime
