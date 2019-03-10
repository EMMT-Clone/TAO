#
# EasyDS9.jl -
#
# Utility routines to facilitate interaction with SAOImage/DS9.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018-2019, Éric Thiébaut and Michel Tallon.
#

module EasyDS9

export
    ds9circle,
    ds9connect,
    ds9get,
    ds9point,
    ds9set,
    ds9show

using XPA, DS9

const ds9connect = DS9.connect
const ds9set = DS9.set
const ds9get = DS9.get

"""
```julia
ds9show(img)
```

sends the image `img` to be displayed in SAOImage/DS9.

Keyword `frame` can be used to specify the frame number.

Keyword `cmap` can be used to specify the name of the colormap.

Keyword `zoom` can be used to specify the zoom factor.

Keywords `min` and/or `max` can be used to specify the scale limits.

"""
function ds9show(img::AbstractMatrix;
                 min=nothing, max=nothing,
                 cmap=nothing, frame=nothing, zoom=nothing)
    # FIXME: pack all commands into a single one.
    frame === nothing || ds9set("frame", frame)
    zoom === nothing || ds9set("zoom to", zoom)
    ds9set(img)
    if min !== nothing || max !== nothing
        ds9set("scale limits",
               (min === nothing ? minimum(img) : min),
               (max === nothing ? maximum(img) : max))
    end
    cmap === nothing || ds9set("cmap", cmap)
    return nothing
end

# Keyword `shape` can be "circle", "box", "diamond", "cross", "x", "arrow"
# or "box"

"""

```julia
ds9circle(x, y)
```

draw a circle at position `(x,y)` in current SAOImage/DS9 frame.  Keywords
`radius`, `color` and `width` can be used to set the attributes of the circle.

Arguments `x` and `y` may be vectors (of same length) to draw multiple circles
with the same attributes.

The coordinates may be packed:

```julia
ds9circle(xy)
```

where `xy` is a 2-element vector or tuple to draw a single circle or matrix of
size `(2,n)` to draw `n` circles (with the same attributes).

See [`ds9point`](@ref).

"""
function ds9circle end

"""

```julia
ds9point(x, y)
```

draw a point at position `(x,y)` in current SAOImage/DS9 frame.  Keywords
`shape`, `size`, `color` and `width` can be used to set the attributes of the
point.

Arguments `x` and `y` may be vectors (of same length) to draw multiple points
with the same attributes.

The coordinates may be packed:

```julia
ds9point(xy)
```

where `xy` is a 2-element vector or tuple to draw a single point or matrix of
size `(2,n)` to draw `n` points (with the same attributes).

See [`ds9circle`](@ref).

"""
function ds9point end

for id in ("circle", "point")
    fn  = Symbol("ds9", id)
    opt = Symbol("_region_", id)
    @eval begin

        function $fn(x::Real, y::Real; kwds...)
            pfx, sfx = $opt(; kwds...)
            ds9set(pfx, x, y, sfx)
            nothing
        end

        $fn(xy::Tuple{Real,Real}; kwds...) =
            $fn(xy[1], xy[2]; kwds...)

        $fn(xy::AbstractVector{<:Real}; kwds...) =
            (@assert length(xy) == 2; $fn(xy[1], xy[2]; kwds...))

        # For multiple points/circles/... we just send a DS9 command for each
        # item to draw.  Packing multiple commands (separated by semi-columns)
        # does not really speed-up things and is more complicated because there
        # is a limit to the total length of an XPA command (given by
        # XPA.SZ_LINE I guess).

        function $fn(x::AbstractVector{<:Real},
                     y::AbstractVector{<:Real}; kwds...)
            @assert length(x) == length(y)
            pfx, sfx = $opt(; kwds...)
            for i in eachindex(x, y)
                ds9set(pfx, x[i], y[i], sfx)
            end
            nothing
        end

        function $fn(xy::AbstractMatrix{<:Real}; kwds...)
            @assert size(xy, 1) == 2
            pfx, sfx = $opt(; kwds...)
            for i in 1:size(xy, 2)
                ds9set(pfx, xy[1,i], xy[2,i], sfx)
            end
            nothing
        end

    end
end

function _region_circle(;
                        radius::Real = 5,
                        color::Union{Nothing,String} = "plum",
                        width::Union{Nothing,Real} = nothing,
                        movable::Bool = false,
                        selectable::Bool = false,
                        fill::Bool = false)
    str = string(radius, " #")
    color === nothing || (str *= string(" color=", color))
    width === nothing || (str *= string(" width=", width))
    return ("regions command { circle", str*" }")
end

function _region_point(;
                       shape::String = "x",
                       size::Real = 11,
                       color::Union{Nothing,String} = "orange",
                       width::Union{Nothing,Real} = nothing,
                       movable::Bool = false)
    str = string("# point=", shape, " ", size)
    color === nothing || (str *= string(" color=", color))
    width === nothing || (str *= string(" width=", width))
    return ("regions command { point", str*" }")
end

function __init__()
    try
        if DS9.accesspoint() == ""
            DS9.connect()
        end
    catch err
        @warn "Failed to automatically connect to SAOImage/DS9.  Launch ds9 then do `ds9connect()`"
    end
end

end
