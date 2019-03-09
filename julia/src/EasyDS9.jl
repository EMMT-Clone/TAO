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
    ds9circles,
    ds9connect,
    ds9get,
    ds9points,
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
function ds9points(X::AbstractVector{<:Real},
                   Y::AbstractVector{<:Real};
                   shape::String = "x",
                   size::Real = 7,
                   color::Union{Nothing,String} = nothing,
                   width::Union{Nothing,Real} = nothing,
                   movable::Bool = false)
    @assert length(X) == length(Y)
    cmd = ["regions command { point", "x", "y",
           string("# point=", shape, " ", size)]
    color === nothing || push!(cmd, string("color=", color))
    width === nothing || push!(cmd, string("width=", width))
    push!(cmd, "}")
    for i in eachindex(X, Y)
        cmd[2] = string(X[i])
        cmd[3] = string(Y[i])
        ds9set(join(cmd, " "))
    end
    return nothing
end

function ds9circles(X::AbstractVector{<:Real},
                    Y::AbstractVector{<:Real};
                    radius::Real = 5,
                    color::Union{Nothing,String} = "plum",
                    width::Union{Nothing,Real} = nothing,
                    movable::Bool = false,
                    selectable::Bool = false,
                    fill::Bool = false)
    @assert length(X) == length(Y)
    cmd = ["regions command { circle", "x", "y", string(radius), "#"]
    color === nothing || push!(cmd, string("color=", color))
    width === nothing || push!(cmd, string("width=", width))
    push!(cmd, "}")
    for i in eachindex(X, Y)
        cmd[2] = string(X[i])
        cmd[3] = string(Y[i])
        ds9set(join(cmd, " "))
    end
    return nothing
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
