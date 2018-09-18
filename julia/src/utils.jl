#
# utils.jl --
#
# Utilities and miscellaneous methods for Julia interface to TAO.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO library (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018, Éric Thiébaut.
#

function TimeStamp(secs::AbstractFloat)
    ts_sec = floor(Int64, secs)
    ts_nsec = round(Int64, (secs - ts_sec)*1E9)
    return TimeStamp(ts_sec, ts_nsec)
end

TimeStamp(secs::Integer) = TimeStamp(secs, 0)
