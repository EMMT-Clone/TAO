#
# config.jl -
#
# Routines for saving/loading Themis adaptive optics configuration.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018-2019, Éric Thiébaut and Michel Tallon.
#

module Config

using Dates, Fits, FITSIO
using LinearInterpolators.AffineTransforms

using ..Themis
import ..Themis:
    DeformableMirror,
    ShackHartmann,
    Spot,
    SubPupil,
    Configuration,
    WavefrontSensor,
    find,
    load,
    save,
    save!

function DeformableMirror(
    ::Type{T},
    shape::AbstractMatrix{Bool}=Themis.DM_SHAPE) where {T<:AbstractFloat}

    return DeformableMirror{T}(
        copyto!(Matrix{Bool}(undef, size(shape)), shape),
        zeros(T, count(shape)))
end

DeformableMirror(shape::AbstractMatrix{Bool}=Themis.DM_SHAPE) =
    DeformableMirror(Cdouble, shape)

ShackHartmann(::Type{T} = Cdouble) where {T<:AbstractFloat} =
    ShackHartmann{T}((0,0),
                     (0,0),
                     Matrix{Bool}(undef, 0, 0),
                     SubPupil{T}[],
                     AffineTransform2D{T}(),
                     AffineTransform2D{T}(),
                     Matrix{T}(undef, 0, 0),
                     Matrix{T}(undef, 0, 0),
                 0, 0, 0)

Configuration(::Type{T} = Cdouble) where {T<:AbstractFloat} =
    Configuration(ShackHartmann{T},
              DeformableMirror{T})

function SubPupil(cfg::Configuration{T},
                  xmin::Integer, xmax::Integer,
                  ymin::Integer, ymax::Integer,
                  xref::Real = T((xmax - xmin)/2 + 1),
                  yref::Real = T((ymax - ymin)/2 + 1)) where {T<:AbstractFloat}
    # Bounding box coordinates are relative to the WFS image.
    width, height = cfg.dims
    @assert 1 ≤ xmin ≤ xmax ≤ width
    @assert 1 ≤ ymin ≤ ymax ≤ height

    # Reference coordinates are relative to the subimage.
    @assert 1 ≤ xref ≤ xmax - xmin + 1
    @assert 1 ≤ yref ≤ ymax - ymin + 1

    return SubPupil(CartesianIndices((xmin:xmax, ymin:ymax)),
                    Spot{T}(xref, yref))
end

nsubs(cfg::Configuration) = length(cfg.subpupils)

# FIXME: add haskey(::FITS, ::AbstractString, ...)

"""
    Themis.save(dest, cfg)
    Themis.save!(dest, cfg)

save Shack-Hartmann wavefront sensor settings `cfg` into destination `dest`.
If `dest` is the name of an existing file, `save` will refuse to overwrite it
while `save!` will overwrite it.  The `save!` method also accepts a FITS handle
for `dest`.

See also: [`ShackHartmann.load`](@ref).

"""
function save(filename::AbstractString, # destination
              cfg::Configuration;           # source
              kwds...)
    exists(filename) && error("file already exists (consider using `save!`)")
    save!(filename, cfg; kwds...)
end

function save!(filename::AbstractString, # destination
               cfg::Configuration;           # source
               kwds...)
    handle = FITS(filename, "w")
    try
        save!(handle, cfg; kwds...)
    finally
        close(handle)
    end
    return nothing
end

function save!(handle::FITS,
               cfg::Configuration{T};
               date = now()) where {T}
    dt = DateTime(date)
    _save_subpupils(handle, cfg, dt)
    _save_detector(handle, cfg, dt)
    _save_pupil(handle, cfg, dt)
    _save_commands(handle, cfg, dt)
    nothing
end

function _save_commands(handle::FITS,
                        cfg::Configuration,
                        dt::DateTime)
    # Build header.
    hdr = _new_header()
    save!(hdr, "DATE", dt)
    #FIXME: hdr["COMMENT"] = "This HDU contains the commands of the deformable mirror to fit the reference wavefront."

    # Write "image".
    write(handle, convert(Vector{Float64}, cfg.refcmds);
          name = "DeformableMirrorReferenceCommands",
          ver = 1, header = hdr)
end


function _save_subpupils(handle::FITS,
                         cfg::Configuration,
                         dt::DateTime)
    deg = π/180

    # Create FITS header.
    hdr = _new_header()
    save!(hdr, "WIDTH", getimagewidth(cfg),
            "Size of region of interest along first axis")
    save!(hdr, "HEIGHT", getimageheight(cfg),
            "Size of region of interest along second axis")
    save!(hdr, "GRD2PIX", cfg.gridtopixel)
    save!(hdr, "PIX2GRD", cfg.pixeltogrid)
    save!(hdr, "DATE", dt)

    # Prepare column data.
    n = length(cfg.subpupils)
    data = Dict("XMIN" => [convert(Int32, getxmin(cfg, i)) for i in 1:n],
                "YMIN" => [convert(Int32, getymin(cfg, i)) for i in 1:n],
                "XMAX" => [convert(Int32, getxmax(cfg, i)) for i in 1:n],
                "YMAX" => [convert(Int32, getymax(cfg, i)) for i in 1:n],
                "PEAK" => [getpeak(cfg, i) for i in 1:n],
                "XREF" => [getxref(cfg, i) for i in 1:n],
                "YREF" => [getyref(cfg, i) for i in 1:n],
                "MAJOR" => [getmajor(cfg, i) for i in 1:n],
                "MINOR" => [getminor(cfg, i) for i in 1:n],
                "ORIENT" => [getorient(cfg, i)/deg for i in 1:n])
    units = Dict("XMIN" => "pixels",
                 "YMIN" => "pixels",
                 "XMAX" => "pixels",
                 "YMAX" => "pixels",
                 "PEAK" => "",
                 "XREF" => "pixels",
                 "YREF" => "pixels",
                 "MAJOR" => "pixels",
                 "MINOR" => "pixels",
                 "ORIENT" => "deg")

    # Write table.
    write(handle, data; hdutype = TableHDU,
          name = "ShackHartmannSubpupils",
          ver = 1, header = hdr, units = units)
end

function _save_detector(handle::FITS,
                        cfg::Configuration,
                        dt::DateTime)
    # Create FITS header.
    hdr = _new_header()
    save!(hdr, "XOFFSET", getxoff(cfg),
          "Offset of region of interest along first axis")
    save!(hdr, "YOFFSET", getyoff(cfg),
          "Offset of region of interest along second axis")
    save!(hdr, "GAIN", getgain(cfg),
          "Detector gain parameter")
    save!(hdr, "BIAS", getbias(cfg),
          "Detector bias or black-level parameter")
    save!(hdr, "RON", getron(cfg),
          "RMS of readout noise [counts/pixel/frame]")
    save!(hdr, "DATE", dt)

    # Write "image".
    write(handle, cat(3, cfg.a, cfg.b),
          name = "ShackHartmannDetector",
          ver = 1, header = hdr)
end

# Create FITS header (beware not to use the same array for the different
# parts of the header).
_new_header() =
    FITSHeader(Array{String}(0), [], Array{String}(0))

function _save_pupil(handle::FITS,
                     cfg::Configuration,
                     dt::DateTime)
    # Build header.
    hdr = _new_header()
    save!(hdr, "DATE", dt)
    #FIXME: hdr["COMMENT"] = "This HDU contains an image of the shape of the Shack-Hartmann wavefront pupil."

    # Write "image".
    write(handle, convert(Matrix{UInt8}, cfg.pupil);
          name = "ShackHartmannPupil",
          ver = 1, header = hdr)
end

function load(::Type{Configuration{T}},
              filename::AbstractString) where {T<:AbstractFloat}
    local cfg
    handle = FITS(filename, "r")
    # FIXME: use do block
    try
        cfg = load(Configuration{T}, handle)
    finally
        close(handle)
    end
    return cfg
end

function load(::Type{Configuration{T}},
              handle::FITS) where {T<:AbstractFloat}
    deg = π/180

    cfg = Configuration(T)
    wfs = cfg.wfs

    # FIXME: type stability issue with `hdu`?

    # Read the wavefront sensor sub-pupils settings.
    hdu = find(TableHDU, handle, "ShackHartmannSubpupils", 1)
    if hdu !== nothing
        hdr = load(FITSHeader, hdu)
        width  = load(Int, hdu, "WIDTH")
        height = load(Int, hdu, "HEIGHT")
        if width < 1 || height < 1
            error("bad wavefront sensor image size")
        end
        wfs.dims = (width, height)
        wfs.gridtopixel = load(AffineTransform2D{T}, hdu, "GRD2PIX")
        if false # FIXME: in case transformation was not invertible
            wfs.pixeltogrid = inv(wfs.gridtopixel)
        end
        xmin   = load(Vector{Int}, hdu, "XMIN")
        ymin   = load(Vector{Int}, hdu, "YMIN")
        xmax   = load(Vector{Int}, hdu, "XMAX")
        ymax   = load(Vector{Int}, hdu, "YMAX")
        xref   = load(Vector{T},   hdu, "XREF")
        yref   = load(Vector{T},   hdu, "YREF")
        major  = load(Vector{T},   hdu, "MAJOR")
        minor  = load(Vector{T},   hdu, "MINOR")
        orient = load(Vector{T},   hdu, "ORIENT")
        peak = (haskey(hdr, "PEAK")
                ? load(Vector{T}, hdu, "PEAK")
                : ones(size(xref)))
        if ! (length(xref) == length(yref) ==
              length(xmin) == length(ymin) ==
              length(xmax) == length(ymax) ==
              length(major) == length(minor) ==
              length(orient) == length(peak))
            error("bad numbers of sub-pupils")
        end
        subpupils = Vector{SubPupil{T}}(undef, length(xmin))
        for i in eachindex(subpupils, xmin, xmin, ymin, ymax)
            subpupils[i] = SubPupil(wfs, xmin[i], xmin[i], ymin[i], ymax[i])
        end
        wfs.subpupils = subpupils
    end

    # Read the wavefront sensor detector settings.
    hdu = find(TableHDU, handle, "ShackHartmannDetector", 1)
    if hdu !== nothing
        if (load(Int, hdu, "NAXIS1") != width ||
            load(Int, hdu, "NAXIS2") != height ||
            load(Int, hdu, "NAXIS3") != 2)
            error("expecting $(width)×$(height)×2 IMAGE data")
        end
        xoff = load(Int, hdu, "XOFFSET")
        yoff = load(Int, hdu, "YOFFSET")
        wfs.offs = (xoff, yoff)
        wfs.gain = load(T, hdu, "GAIN")
        wfs.bias = load(T, hdu, "BIAS")
        wfs.ron  = load(T, hdu, "RON")
        arr = load(Array, hdu)
        wfs.a = arr[:,:,1] # gain correction
        wfs.b = arr[:,:,2] # bias correction
    end

    #FIXME: # Read the mirror commands for the reference.
    #FIXME: hdu = find(TableHDU, handle, "DeformableMirrorReferenceCommands", 1)
    #FIXME: if hdu !== nothing
    #FIXME:     refcmds = read(hdu)
    #FIXME:
    #FIXME:     # Build sub-pupils part.
    #FIXME:     n = length(xref)
    #FIXME:     subpupils = Vector{SubPupil{T}}(n)
    #FIXME:     for k in 1:n
    #FIXME:         box = CartesianIndices((xmin[k] : xmax[k],
    #FIXME:                                 ymin[k] : ymax[k]))
    #FIXME:         ref = Spot(peak[k], xref[k], yref[k], major[k], minor[k], orient[k]*deg)
    #FIXME:         subpupils[k] = SubPupil(box, ref)
    #FIXME:     end
    #FIXME: end

    return cfg
end

function find(::Type{T}, handle::FITS, name::AbstractString,
              ver::Int=0) where {T<:HDU}
    try
        # FIXME: check HDU type
        return handle[name, ver]
    catch err
        if isa(err, ErrorException) && err.msg == "illegal HDU number"
            return nothing
        end
        rethrow(err)
    end
end

function save!(hdr::FITSHeader, key::AbstractString,
               val, com::AbstractString="")
    hdr[key] = val
    set_comment!(hdr, key, com)
    nothing
end

function save!(hdr::FITSHeader, key::AbstractString,
               dt::DateTime, com::AbstractString="")
    save!(hdr, key, string(round(dt, Dates.Second)), com)
end

function save!(hdr::FITSHeader, prefix::AbstractString, A::AffineTransform2D)
    save!(hdr, prefix*"1", A.xx, "1st coef. of affine transform")
    save!(hdr, prefix*"2", A.xy, "2nd coef. of affine transform")
    save!(hdr, prefix*"3", A.x,  "3rd coef. of affine transform")
    save!(hdr, prefix*"4", A.yx, "4th coef. of affine transform")
    save!(hdr, prefix*"5", A.yy, "5th coef. of affine transform")
    save!(hdr, prefix*"6", A.y,  "6th coef. of affine transform")
    nothing
end

load(::Type{FITSHeader}, hdu::HDU) = read_header(hdu)

load(::Type{Array}, hdu::HDU, args...) = read(hdu, args...)

function load(::Type{AffineTransform2D{T}}, hdu::HDU,
              pfx::AbstractString) where {T<:AbstractFloat}
    a = [load(T, hdu, string(pfx,i)) for i in 1:6]
    AffineTransform2D{T}(a...)
end

load(::Type{T}, hdu::HDU, key::AbstractString) where {T<:Integer} =
    _load_value(T, hdu, key, Integer, "an integer")

load(::Type{T}, hdu::HDU, key::AbstractString) where {T<:AbstractFloat} =
    _load_value(T, hdu, key, Real, "a real")

load(::Type{String}, hdu::HDU, key::AbstractString) =
    _load_value(String, hdu, key, AbstractString, "a string")

load(::Type{Array{T,N}}, hdu::HDU, key::AbstractString) where {T<:Integer,N} =
    _load_column(Array{T,N}, hdu, key, Integer, "integers")

load(::Type{Array{T,N}}, hdu::HDU, key::AbstractString) where {T<:AbstractFloat,N} =
    _load_column(Array{T,N}, hdu, key, Real, "reals")

function _load_value(::Type{T},
                     hdu::HDU,
                     key::AbstractString,
                     ::Type{P}, # parent type or class
                     typ::AbstractString) where {P,T}
    local val
    try
        val = read_key(hdu, key)[1]
    catch err
        error("cannot read \"$key\" [$err]")
    end
    isa(val, P) || error("value of \"$key\" is not $(typ)")
    return convert(T, val)
end

function _load_column(::Type{T},
                      hdu::HDU,
                      key::AbstractString,
                      ::Type{P}, # parent type or class
                      typ::AbstractString) where {P,T<:AbstractArray{<:P}}
    local arr
    try
        arr = read(hdu, key)
    catch err
        error("cannot read column \"$key\" [$err]")
    end
    eltype(arr) <: P || error("values of column \"$key\" are not $(typ)")
    return convert(T, arr)
end

end # module Config
