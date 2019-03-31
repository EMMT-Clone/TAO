#
# sharedcameras.jl --
#
# Management of shared camera data for Julia interface to TAO
# C-library.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018, Éric Thiébaut.
#

function attach(::Type{SharedCamera}, ident::Integer)
    # Attach the shared object to the address space of the caller, then wrap it
    # in a Julia object.
    errs = Errors()
    ptr = ccall((:tao_attach_shared_camera, taolib), Ptr{Cvoid},
                (Ref{Errors}, Cint), errs, ident)
    _check(ptr != C_NULL, errs)
    return _wrap(SharedCamera(), ptr)
end

for (T, m) in ((Cint,    :state),
               (Cint,    :pixeltype),
               (Cint,    :depth),
               (Clong,   :sensorwidth),
               (Clong,   :sensorheight),
               (Clong,   :xoff),
               (Clong,   :yoff),
               (Clong,   :width),
               (Clong,   :height),
               (Cdouble, :bias),
               (Cdouble, :gain),
               (Cdouble, :framerate),
               (Cdouble, :exposuretime),
               (Cdouble, :gamma))
    jf = Symbol("get_", m)
    cf = string("tao_get_shared_camera_", m)
    @eval $jf(cam::SharedCamera) = ccall(($cf, taolib), $T, (Ptr{Cvoid},), cam)
end

get_last_image_counter(cam::SharedCamera) =
    ccall((:tao_get_last_image_counter, taolib), Int64, (Ptr{Cvoid},), cam)

get_last_image_ident(cam::SharedCamera) =
    ccall((:tao_get_last_image_ident, taolib), Cint, (Ptr{Cvoid},), cam)
