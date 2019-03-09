#
# Themis.jl --
#
# Julia module Themis for Themis adaptive optics software.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018-2019, Éric Thiébaut & Michel Tallon.
#

module Themis

export
    find,
    load,
    save,
    save!

using FITSIO
using LinearInterpolators.AffineTransforms

# `find`, `load` and `save`, `save!` are not part of the base methods so
# they can be specialized to match our needs.
function find end
function load end
function save! end
function save end

include("themis/types.jl")
include("themis/utils.jl")
include("themis/config.jl")

end # module Tao
