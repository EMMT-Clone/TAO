#
# errors.jl --
#
# Management of errors for Julia interface to TAO.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO library (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018, Éric Thiébaut.
#

@inline _check(errs::Errors) =
    any_errors(errs) && throw(TaoError(pop_errors(errs)))

function _check(success::Bool, errs::Errors)
    if any_errors(errs)
        if success
            @warn "there should be no errors"
            discard_errors(errs)
        else
            throw(TaoError(pop_errors(errs)))
        end
    elseif ! success
        @error "failure without error information"
    end
end

"""

`TAO.get_error_reason(err)` yields the error message associated to `err` which
is either an instance of `TAO.ErrorInfo` or an integer error code.

"""
get_error_reason(err::ErrorInfo) = get_error_reason(err.code)
get_error_reason(code::Integer) =
    unsafe_string(ccall((:tao_get_error_reason, taolib), Ptr{UInt8},
                        (Cint,), code))

"""

`TAO.get_error_name(err)` yields the literal name associated to `err`
which is either an instance of `TAO.ErrorInfo` or an integer error code.

"""
get_error_name(err::ErrorInfo) = get_error_name(err.code)
get_error_name(code::Integer) =
    unsafe_string(ccall((:tao_get_error_name, taolib), Ptr{UInt8},
                        (Cint,), code))

"""

`TAO.any_errors(errs)` yields whether there are any errors in `errs`.

"""
@inline any_errors(errs::Errors) = (errs.ptr != C_NULL)

"""

`TAO.report_errors(errs)` prints the errors in `errs` to the standard error
output and clear the contents of errs.

"""
report_errors(errs::Errors) =
    ccall((:tao_report_errors, taolib), Cvoid, (Ref{Errors},), errs)

"""

`TAO.discard_errors(errs)` discards the errors in `errs`.

"""
discard_errors(errs::Errors) =
    ccall((:tao_discard_errors, taolib), Cvoid, (Ref{Errors},), errs)

"""

`TAO.pop_errors(errs)` pops the errors from `errs` and returns an array of
instances of `TAO.ErrorInfo` (possibly empty if there are no errors).

"""
function pop_errors(errs::Errors)
    func = Ref{Ptr{UInt8}}(0)
    code = Ref{Cint}(0)
    info = Array{ErrorInfo}(undef, 0)
    while 0 != ccall((:tao_pop_error, taolib), Cint,
                     (Ref{Errors}, Ptr{Ptr{UInt8}}, Ptr{Cint}),
                     errs, func, code)
        push!(info, ErrorInfo(unsafe_string(func[]), code[]))
    end
    return info
end
