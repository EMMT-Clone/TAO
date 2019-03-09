#
# Fits.jl -
#
# Utility routines to simplify reading/writing FITS files.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018-2019, Éric Thiébaut and Michel Tallon.
#

module Fits

export
    exists,
    loadfits,
    openfits

using FITSIO

import Base: getindex, setindex!, keys

"""

```julia
exists(path) -> boolean
```

yields whether file `path` exists.  Argument can be a file name or an instance
of `Base.Filesystem.StatStruct` as returned by the `stat` method.

"""
exists(st::Base.Filesystem.StatStruct) = (st.nlink != 0)
exists(path) = exists(stat(path))

struct Data{T,N}
    hdr::FITSHeader
    arr::Array{T,N}
end

getheader(dat::Data) = dat.hdr
getdata(dat::Data) = dat.arr
getcomment(dat::Data, k) = FITSIO.get_comment(getheader(dat), k)

FITSIO.get_comment(dat::Data, k) = getcomment(dat, k)

getindex(dat::Data, k::AbstractString) = getindex(getheader(dat), k)
getindex(dat::Data, inds...) = getindex(getdata(dat), inds...)
setindex!(dat::Data, val, inds...) = setindex(getdata(dat), val, inds...)
keys(dat::Data) = keys(getheader(dat))
nkeys(dat::Data) = nkeys(getheader(dat))
nkeys(hdr::FITSHeader) = length(hdr)

"""

```julia
openfits(filename) -> fh
```

yields a `FITS` handle to read the contents of the existing FITS file whose
name is `filename`.  If `filename` does not exists but `filename` does not end
with the `".gz"` extension and `"\$filename.gz"` does exist, then the
compressed file `"\$filename.gz"` is open instead.

See also: [`loadfits`](@ref).

"""
openfits(filename::AbstractString) =
    FITS(exists(filename) || endswith(filename, ".gz") ||
         !exists(filename*".gz") ? filename : filename*".gz")

"""

```julia
loadfits(arg, hdu=1) -> A
```

yields a pseudo-array `A` with the contents of the FITS HDU (*header data
unit*) `hdu` in `arg`.  Argument `arg` can be the name of a FITS file or a FITS
handle.  The optional HDU number, the first one by default, must correspond to
a FITS *IMAGE* extension.  The result is indexable.  Using string index yields
the value of the corresponding FITS keyword in the header part of the HDU.  Any
other indices are used to access the contents of data part of the HDU (as a
regular Julia array).

Examples:

```julia
using Fits
A = loadfits("image.fits")   # load the first HDU
A[2,3]                       # get value of data at indices (2,3)
A["BITPIX"]                  # get FITS bits per pixel
Fits.getcomment(A, "BITPIX") # get the associated comment
arr = Fits.getdata(A)        # get the data part (a regular Julia array)
hdr = Fits.getheader(A)      # get the header part
Fits.nkeys(A)                # get the number of keywords
Fits.nkeys(hdr)              # get the number of keywords
keys(A)                      # get the list of keywords
keys(hdr)                    # get the list of keywords
```

See also: [`openfits`](@ref).

"""
loadfits(filename::AbstractString, hdu::Integer=1) =
    loadfits(openfits(filename), hdu)

loadfits(fh::FITS, hdu::Integer=1) =
    Data(read_header(fh[hdu]), read(fh[hdu]))

end # module
