#
# types.jl --
#
# Type definitions for julia module Themis.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018-2019, Éric Thiébaut and Michel Tallon.
#

"""

A `Spot` structure stores the parameters of a spot image at the focus of a
given lens of the wavefront sensor.

"""
struct Spot{T <: AbstractFloat} # <: AbstractPoint{T}
    #p::T # peak value
    x::T # abscissa
    y::T # ordinate
    #a::T # standard deviation along major axis
    #b::T # standard deviation along minor axis
    #θ::T # angle of major axis with first axis in radians counterclockwise
end

struct SubPupil{T<:AbstractFloat, B<:CartesianIndices{2}}
    box::B              # bounding box of the sub-pupil image
    ref::Spot{T}        # reference position
    #dat::Vector{T}      # workspace array for the pixel values of the box
    #var::Vector{T}      # workspace array for the pixel variances in the box
    #wgt::Vector{T}      # workspace array for the pixel weights in the box
    #mod1::Vector{T}     # 1st mode of the model in the box
    #mod2::Vector{T}     # 2nd mode of the model in the box
    #mod3::Vector{T}     # 3rd mode of the model in the box
end

abstract type WavefrontSensor{T<:AbstractFloat} end

mutable struct ShackHartmann{T<:AbstractFloat} <: WavefrontSensor{T}
    dims::NTuple{2,Int} # size of wavefront sensor images
    offs::NTuple{2,Int} # offsets of wavefront sensor images relative to captor
    shape::Matrix{Bool} # pupil shape
    subpupils::Vector{SubPupil{T}}
    gridtopixel::AffineTransform2D{T} # convert grid coord. into pixel coord.
    pixeltogrid::AffineTransform2D{T} # convert pixel coord. into grid coord.
    a::Matrix{T}  # image gain correction
    b::Matrix{T}  # image bias correction
    ron::T         # detector readout noise (rms in counts/pixel/frame)
    bias::T        # bias / black-level detector parameter
    gain::T        # gain detector parameter
end

mutable struct DeformableMirror{T<:AbstractFloat}
    shape::Matrix{Bool} # mirror shape
    refcmds::Vector{T}  # reference commands
end

mutable struct Configuration{T<:AbstractFloat}
    wfs::ShackHartmann{T}
    dm::DeformableMirror{T}
end
