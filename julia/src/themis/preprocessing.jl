#
# themis/preprocessing.jl --
#
# Preprocessing of images.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018-2019, Éric Thiébaut & Michel Tallon.
#

function preprocessing(raw::DenseMatrix{<:Integer},
                       bias::DenseMatrix{T},
                       gain::DenseMatrix{T}) where {T<:AbstractFloat}
    return preprocessing!(Array{T,2}(undef, size(raw)), raw, bias, gain)
end

function preprocessing!(dest::DenseMatrix{T},
                        raw::DenseMatrix{<:Integer},
                        bias::DenseMatrix{T},
                        gain::DenseMatrix{T}) where {T<:AbstractFloat}
    @assert size(dest) == size(raw) == size(bias) == size(gain)
    @inbounds @simd for k in 1:length(dest)
        dest[k] = (raw[k] - bias[k])*gain[k]
    end
    return dest
end


# Always make a copy so that the raw image can be released.
# FIXME: With the future API of ScientificCameras, this should not be needed
#        because `release(cam)` will be a no-op and thus deprecated.
preprocessing(::Type{T}, raw::DenseMatrix{<:Integer}) where {T<:Real} =
    preprocessing!(Array{T,2}(undef, size(raw)), raw)

function preprocessing!(dest::DenseMatrix{<:Real},
                        raw::DenseMatrix{<:Integer})
    # Note: copyto!() does not check dimensions.
    @assert size(dest) == size(raw)
    @inbounds @simd for k in 1:length(dest)
        dest[k] = raw[k]
    end
    return dest
end
