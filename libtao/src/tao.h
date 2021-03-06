/*
 * tao.h --
 *
 * Definitions for TAO (TAO is a library for Adaptive Optics software).
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018-2019, Éric Thiébaut.
 */

#ifndef _TAO_H_
#define _TAO_H_ 1

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

/**
 * @defgroup Messages             Messages and logging
 * @defgroup Errors               Error reporting
 * @defgroup Buffers              Input/output buffers
 * @defgroup Arrays               Multi-dimensional arrays
 * @defgroup SharedObjects        Shared objects
 * @defgroup   BasicSharedObjects Basic shared objects
 * @defgroup   SharedArrays       Shared multi-dimensional arrays
 * @defgroup Utilities            Utility functions
 * @defgroup   Commands           Parsing of commands
 * @defgroup   DynamicMemory      Dynamic memory
 * @defgroup   Locks              Locks
 * @defgroup   Time               Date and time
 * @defgroup   Macros             Useful macros
 */

#ifdef __cplusplus
#  define _TAO_BEGIN_DECLS extern "C" {
#  define _TAO_END_DECLS }
#else
#  define _TAO_BEGIN_DECLS
#  define _TAO_END_DECLS
#endif

_TAO_BEGIN_DECLS

/*---------------------------------------------------------------------------*/

/**
 * @addtogroup Messages
 *
 * @{
 */

typedef enum tao_message_type {
    /* Values must be in ascending order. */
    TAO_MESG_DEBUG  = 0,  /**< Debug message */
    TAO_MESG_INFO   = 1,  /**< Information message */
    TAO_MESG_WARN   = 2,  /**< Warning message */
    TAO_MESG_ERROR  = 3,  /**< Runtime error */
    TAO_MESG_ASSERT = 4,  /**< Assertion error or bug */
    TAO_MESG_QUIET  = 5   /**< Suppress all messages */
} tao_message_type_t;

/**
 * Print a formatted message.
 *
 * Depending on the current setting for printing messages, this function
 * either does nothing, or print a formatted message.
 *
 * This function is thread-safe.
 *
 * @param type    The type of message.  The message is printed if the type
 *                of the message is at least as important as the current
 *                minimum message level set by tao_set_message_level().
 *
 * @param format  A format string (as for `printf`).
 *
 * @param ...     The arguments.
 */
extern void tao_inform(tao_message_type_t type, const char* format, ...)
    __attribute__ ((format (printf, 2, 3)));

/**
 * Get the minimum level of printed messages.
 *
 * This function is thread-safe.
 *
 * @return The current minimum level for messages printed by tao_inform().
 */
extern tao_message_type_t tao_get_message_level();

/**
 * Set the minimum level of printed messages.
 *
 * This function is thread-safe.
 *
 * @param level   The minimum level of messages printed by tao_inform().  For
 *                instance, `TAO_MESG_QUIET` to suppress printing of messages or
 *                `TAO_MESG_DEBUG` to print everything.
 */
extern void tao_set_message_level(tao_message_type_t level);

/** @} */

/*---------------------------------------------------------------------------*/

/**
 * @addtogroup Errors
 *
 * Management of errors.
 *
 * Errors in TAO are identified by a numerical code (see `tao_error_code_t`)
 * and by the name of the function where the error occured.  When an error
 * occurs in some TAO function, this information may be transferred to the
 * caller via the first parameter of the function.  (Note: it must be the first
 * argument because there are functions with a variable number of arguments.)
 * To collect error information, the caller has to provide the address of a
 * pointer to an opaque `tao_error_t` structure.  There are two possibilities:
 *
 * - If the address of the variable to track the errors is `NULL`, then any
 *   occuring error is assumed to be fatal: it is immediately reported and the
 *   process is aborted.
 *
 * - If the address of the variable to track the errors is not `NULL`, then
 *   information about errors that have occured are stored (as a chained list)
 *   in the variable.  Initially, the variable to track error information must
 *   be set to `TAO_NO_ERRORS` to indicate that there are no errors yet.  To
 *   check whether an error has occured, the caller just have to check whether
 *   the variable is not equal to `TAO_NO_ERRORS` after calling a TAO function.
 *   (Some TAO functions may also return a specific value, say `NULL` or `-1`,
 *   when an error occured.)  The macro TAO_ANY_ERRORS() can also be used to
 *   check whether any error occured.  It is the caller responsibility to
 *   ensure that errors are eventually taken into account.  There are basically
 *   two possibilities: errors can be reported with tao_report_errors() or
 *   errors can be simply ignored with the tao_discard_errors().  Calling any
 *   of these two functions does free the resources associated with the
 *   memorized errors.
 *
 * Note that, if the (small) amount of memory needed to store the error
 * information cannot be allocated, a fatal error is assumed: the current error
 * and all pending errors are immediately reported and the process is aborted.
 *
 * Usually, functions should stop upon the first ocurring error.  This is
 * however not always possible (for instance when an object is destroyed
 * several resources may have to be destroyed).  When there are several errors
 * occuring in the same function call, the error mangement mechanism
 * implemented in TAO offers the possibility to report all errors that occured.
 *
 * @{
 */

/**
 * Opaque structure to track error information.
 */
typedef struct tao_error tao_error_t;

/**
 * Initial value of pointer for tracking error information.
 */
#define TAO_NO_ERRORS ((tao_error_t*)0)

/**
 * Check whether any errors occured.
 *
 * @param errs   Address of variable to track errors.
 *
 * @return A boolean result.
 */
#define TAO_ANY_ERRORS(errs) ((errs) != NULL && *(errs) != TAO_NO_ERRORS)

/**
 * Error codes.
 *
 * Errors in TAO are identified by an integer.  Strictly positive values
 * indicate a system error using the same identifiers as `errno`.  Strictly
 * negative values indicate a TAO error.  Zero (that is, `TAO_SUCCESS`)
 * indicate a successful operation.
 */
typedef enum tao_error_code {
    TAO_SUCCESS             =   0, /**< Operation was successful */
    TAO_ACQUISITION_RUNNING =  -1, /**< Acquisition running */
    TAO_ALREADY_EXIST       =  -2, /**< Destination already exists */
    TAO_ALREADY_IN_USE      =  -3, /**< Resource already in use */
    TAO_ASSERTION_FAILED    =  -4, /**< Assertion failed */
    TAO_BAD_ADDRESS         =  -5, /**< Invalid address */
    TAO_BAD_ARGUMENT        =  -6, /**< Invalid argument */
    TAO_BAD_BIAS            =  -7, /**< Invalid detector bias */
    TAO_BAD_CHANNELS        =  -8, /**< Invalid number of channels */
    TAO_BAD_CHARACTER       =  -9, /**< Illegal character */
    TAO_BAD_DEPTH           = -10, /**< Invalid bits per pixel */
    TAO_BAD_DEVICE          = -11, /**< Invalid device */
    TAO_BAD_ENCODING        = -12, /**< Bad encoding */
    TAO_BAD_ESCAPE          = -13, /**< Unknown escape sequence */
    TAO_BAD_EXPOSURETIME    = -14, /**< Invalid exposure time */
    TAO_BAD_GAIN            = -15, /**< Invalid detector gain */
    TAO_BAD_MAGIC           = -16, /**< Invalid magic number */
    TAO_BAD_NAME            = -17, /**< Bad parameter name */
    TAO_BAD_RANK            = -18, /**< Invalid number of dimensions */
    TAO_BAD_FRAMERATE       = -19, /**< Invalid acquistion frame rate */
    TAO_BAD_ROI             = -20, /**< Invalid region of interest */
    TAO_BAD_SERIAL          = -21, /**< Invalid serial number */
    TAO_BAD_SIZE            = -22, /**< Invalid size */
    TAO_BAD_SPEED           = -23, /**< Invalid connection speed */
    TAO_BAD_TYPE            = -24, /**< Invalid type */
    TAO_CANT_TRACK_ERROR    = -25, /**< Insufficient memory to track errors */
    TAO_CORRUPTED           = -26, /**< Corrupted structure */
    TAO_DESTROYED           = -27, /**< Resource has been destroyed */
    TAO_MISSING_SEPARATOR   = -28, /**< Separator missing */
    TAO_NOT_FOUND           = -29, /**< Item not found */
    TAO_NOT_READY           = -30, /**< Device not ready */
    TAO_NO_ACQUISITION      = -31, /**< Acquisition not started */
    TAO_NO_FITS_SUPPORT     = -32, /**< Compiled with no FITS support */
    TAO_OUT_OF_RANGE        = -33, /**< Out of range argument */
    TAO_SYSTEM_ERROR        = -34, /**< Unknown system error */
    TAO_UNCLOSED_STRING     = -35, /**< Unclosed string */
    TAO_UNREADABLE          = -40, /**< Not readable */
    TAO_UNSUPPORTED         = -41, /**< Unsupported feature */
    TAO_UNWRITABLE          = -42, /**< Not writable */
} tao_error_code_t;


/**
 * Callback to retrieve error details.
 *
 * Such a callback is called to retrieve details about an error not due to a
 * call of a C library function nor to a TAO library function.
 *
 * @param code     Error code.
 * @param reason   Address of string pointer to store the *reason* of the error.
 * @param info     Address of string pointer to store the textual equivalent of
 *                 the error code.
 *
 * The callback shall set the value pointed by @b reason and/or @b info to the
 * address of a static string if the corresponding information can be provided
 * and to `NULL` otherwise.  Any of these pointers can be `NULL` to indicate
 * that the corresponding information is not requested.
 *
 * The provided information is used to print an error message of the form:
 *
 * > $prefix $reason in `$func` [$info]
 *
 * where `$prefix` is usually the string `"{ERROR}"`, `$reason` is the string
 * provided by the callback (or `"Some error occured"` if `NULL`), `$func` is
 * the name of function which raised the error and `$info` is the string
 * provided by the callback (or the textual value of error code if `NULL`).
 *
 * The following example shows such a callback:
 *
 * ```.c
 * void get_error_details(int code, const char** reason, const char** info)
 * {
 *     *reason = "Some frame grabber error occured";
 *     *info = NULL; // textual value of error code will be used
 * }
 * ```
 *
 * @see tao_retrieve_error_details.
 */
typedef void tao_error_getter_t(int code, const char** reason,
                                const char** info);

/**
 * Register an error due to a foreign function call.
 *
 * This function is called to register the information related to the occurence
 * of an error.  This information consist in the name of the function @b func
 * where the error occured and the numerical identifier @b code of the error.
 * If @b errs is non-`NULL`, it is assumed to be that of the variable provided
 * by the caller to track errors and the error information is added there;
 * othwerise (that is, if @b errs is `NULL`), the error is immediately reported
 * and the process is aborted.  If argument @b proc is `NULL`, it is assumed
 * that the error code follows the convention in TAO library (nonnegative codes
 * are for system errors while strictly negative codes are for errors in TAO
 * functions); otherwise, @b proc is the callback which can be called to
 * retrieve error details from the error code.
 *
 * @warning @b func must be a static string.
 *
 * @param errs   Address of a variable to track errors.
 * @param func   Name of the function where the error occured.
 * @param code   Error identifier.
 * @param proc   Callback to retrieve information (can be `NULL`).
 */
extern void tao_push_other_error(tao_error_t** errs, const char* func,
                                 int code, tao_error_getter_t* proc);

/**
 * Register an error due to a function call.
 *
 * This function is equivalent to:
 *
 * ```.c
 * tao_push_other_error(errs, func, code, NULL);
 * ```
 *
 * @warning @b func must be a static string.
 *
 * @param errs   Address of a variable to track errors.
 * @param func   Name of the function where the error occured.
 * @param code   Error identifier.
 */
extern void tao_push_error(tao_error_t** errs, const char* func, int code);

/**
 * Register an error due to a system function call.
 *
 * This function is equivalent to:
 *
 * ```.c
 * #include <errno.h>
 * tao_push_error(errs, func, errno);
 * ```
 *
 * @param errs   Address of a variable to track errors.
 * @param func   Name of the function where the error occured.
 *
 * @see tao_push_error.
 */
extern void tao_push_system_error(tao_error_t** errs, const char* func);

/**
 * Pop last tracked error.
 *
 * This function pops information about the most recent error remaining in the
 * list of errors tracked by the variable at address @b errs.  The errors are
 * popped in reverse temporal order.  That is, the last occuring error is
 * retrieved first.  Resources associated with the popped error are freed.
 *
 * The following example demonstrates how to use tao_pop_error() to report
 * all errors that occured:
 *
 * ```.c
 * void report_errors(tao_error_t** errs)
 * {
 *     int code;
 *     const char* func;
 *     while (tao_pop_error(errs, &func, &code, NULL)) {
 *         fprintf(stderr, "error %d in %s\n", code, func);
 *     }
 * }
 * ```
 *
 * @param errs      Address of a variable to track errors.
 * @param funcptr   Address of a variable to store the name of the function
 *                  where the most recent remaining error.  Can be `NULL` to
 *                  not retrieve this information.
 * @param codeptr   Address of a variable to store the code of the most
 *                  recent remaining error.  Can be `NULL` to not retrieve
 *                  this information.
 * @param procptr   Address of a variable to store the callback to retrieve
 *                  details about the most recent remaining error.  Can be
 *                  `NULL` to not retrieve this information.
 *
 * @return A boolean value (that is, `0` or `1`) indicating whether there was
 * some error information to retrieve.  When there are no errors, the value at
 * @b codeptr is set to `0` and the values at @b funcptr and @b procptr are set
 * to `NULL` (respectively providing that @b codeptr, @b funcptr and @b procptr
 * are not `NULL`).
 */
extern int tao_pop_error(tao_error_t** errs, const char** funcptr,
                         int* codeptr, tao_error_getter_t** procptr);

/**
 * Transfer errors between two error stacks.
 *
 * This function removes the errors from @b src and stack them int @b dest.  If
 * there are any errors in @b src and @b dest is `NULL`, it is assumed that the
 * caller has declined to store error informations and all errors in @b src are
 * reported and the process aborted.

 * @param dest      Address of a variable tracking errors (can be `NULL`).
 * @param src       Address of another variable tracking errors (can be
 *                  `NULL`).
 */
extern void tao_transfer_errors(tao_error_t** dest, tao_error_t** src);

/**
 * Retrieve details about a given error code.
 *
 * @param code      Error code.
 * @param reason    Address of a variable to store the reason of the error.
 *                  Can be `NULL` to not retrieve this information.
 * @param info      Address of a variable to store a textual description of
 *                  the error code.  Can be `NULL` to not retrieve this
 *                  information.
 * @param proc      Callback to retrieve details about an error given its code.
 *                  Can be `NULL` to assume the convention in TAO library
 *                  (nonnegative codes are for system errors while strictly
 *                  negative codes are for errors in TAO functions).
 * @param buffer    Address of a small text buffer to print the decimal value
 *                  of the error code if no better description can be obtained.
 *                  Providing this buffer is only useful if @b infoptr is not
 *                  `NULL`.  Can be `NULL` to not use this fallback; otherwise
 *                  must have at least 20 characters (enough to print any value
 *                  of a 64-bit signed integer in decimal form).
 *
 * @see tao_error_getter_t.
 */
extern void tao_retrieve_error_details(int code, const char** reason,
                                       const char** info,
                                       tao_error_getter_t* proc, char* buffer);

/**
 * Report all tracked errors.
 *
 * This function prints to the standard error stream the errors in the given
 * list in reverse order of occurence and delete the contents of the error
 * list.
 *
 * @param errs   Address of a variable to track errors.
 */
extern void tao_report_errors(tao_error_t** errs);

/**
 * Clear all tracked errors.
 *
 * This function deletes the contents of the error stack.
 *
 * @param errs   Address of a variable to track errors.
 */
extern void tao_discard_errors(tao_error_t** errs);

/**
 * Get error message.
 *
 * This function yields the error message associated to a given error code.
 *
 * @param code   Error identifier.
 *
 * @return A permanent string.
 *
 * @history This function is based on `Tcl_ErrnoMsg` in the
 * [Tcl/Tk](http://www.tcl.tk) library.
 */
extern const char* tao_get_error_reason(int code);

/**
 * Get human readable error identifier.
 *
 * Given one of the TAO error codes, this function returns a string with the
 * symbolic name of the code.  For instance, `tao_get_error_name(EINVAL)` yields
 * the string `"EINVAL"`.
 *
 * @param code   Error identifier.
 *
 * @return A permanent string.
 *
 * @history This function is based on `Tcl_ErrnoId` in the
 * [Tcl/Tk](http://www.tcl.tk) library.
 *
 * @see tao_get_error_reason.
 */
extern const char* tao_get_error_name(int code);

/** @} */

/*---------------------------------------------------------------------------*/

/**
 * @addtogroup Buffers
 *
 * Input/Output Buffers.
 *
 * I/O buffers are useful to store data of variable size (their contents may be
 * dynamically resized) and which may only be partially transferred during
 * read/write operations.  They are mostly useful for input/output but may be
 * used for other purposes.
 *
 * @{
 */

/**
 * Input/output buffer.
 *
 * This structure is used to buffer input/output (i/o) data.  This structure is
 * exposed so that callers may use static structure.  The members of the
 * structure may however change and users should only use the provided
 * functions to manipulate i/o buffers.
 */
typedef struct tao_buffer {
    char* data;         /**< Dynamic buffer */
    size_t size;        /**< Number of allocated bytes */
    size_t offset;      /**< Offset of first pending byte */
    size_t pending;     /**< Number of pending bytes */
    unsigned int flags; /**< Bitwise flags used internally */
} tao_buffer_t;

/**
 * Initialize a static i/o buffer.
 *
 * Use this function to initialize a static i/o buffer structure.  When no
 * longer needed, the internal resources which may have been allocated must be
 * released by calling tao_finalize_buffer().  The structure itself is assumed
 * static and will not be freed by tao_finalize_buffer() which will reset its
 * contents as if just initialized instead.
 *
 * @param buf   Address of a static i/o buffer structure.
 */
extern void tao_initialize_static_buffer(tao_buffer_t* buf);

/**
 * Create a dynamic i/o buffer.
 *
 * This function creates a new i/o buffer.  Both the container (the buffer
 * structure) and the contents (the data stored by the buffer) will be
 * dynamically allocated.  When no longer needed, the caller is responsible for
 * calling tao_finalize_buffer() to release all the resources allocated for
 * the buffer (that is, the container and the contents).
 *
 * @param errs   Address of a variable to track errors.
 * @param size   Initial number of bytes of the buffer (actual number of
 *               bytes may be larger but not smaller).
 *
 * @return The address of the new buffer; `NULL` in case of errors.
 */
extern tao_buffer_t* tao_create_buffer(tao_error_t** errs, size_t size);

/**
 * Destroy dynamic resources of an i/o buffer.
 *
 * This function frees any dynamic resources used by the i/o buffer @b buf.
 * If the buffer has been initialized by tao_initialize_static_buffer(), only
 * the contents of the buffer may be destroyed and the buffer is reset to have
 * an empty contents, just as done by tao_initialize_static_buffer(), and can
 * be safely re-used.  If the buffer has been created by tao_create_buffer(),
 * the contents and the container (that is, the structure itself) are destroyed
 * and @b buf must no longer be used.
 *
 * @param buf    Address of the i/o buffer to destroy (can be `NULL`).
 */
extern void tao_finalize_buffer(tao_buffer_t* buf);

/**
 * Resize an i/o buffer.
 *
 * This function has to be called to ensure that a given number of unused bytes
 * are available after the end of the contents (the pending data) stored by an
 * i/o buffer.
 *
 * This function checks the consistency of the buffer structure but, otherwise,
 * tries to make the minimal effort to ensure that the requested number of
 * unused bytes are available.  As needed, it may flush or re-allocate the
 * internal data buffer.  The size of the internal data can never decrease when
 * calling this function.
 *
 * @param errs   Address of a variable to track errors.
 * @param buf    Address of the i/o buffer.
 * @param cnt    Minimum size (in bytes) of unused space.
 *
 * @return `0` on success, `-1` on error.
 */
extern int tao_resize_buffer(tao_error_t** errs, tao_buffer_t* buf, size_t cnt);

/**
 * Flush the contents of an i/o buffer.
 *
 * This function moves the contents (the pending data) of an i/o buffer to the
 * beginning of its internal storage area so as to left the maximum possible
 * amount of unused bytes after the pending bytes.
 *
 * @warning This function is meant to be fast.  It assumes its argument is
 * correct and makes no error checking.
 *
 * @param buf    Address of the i/o buffer.
 */
extern void tao_flush_buffer(tao_buffer_t* buf);

/**
 * Clear the contents of an i/o buffer.
 *
 * This function drops all the contents (the pending data) of an i/o buffer.
 *
 * @warning This function is meant to be fast.  It assumes its argument is
 * correct and makes no error checking.
 *
 * @param buf    Address of the i/o buffer.
 */
extern void tao_clear_buffer(tao_buffer_t* buf);

/**
 * Get the size of the contents of an i/o buffer.
 *
 * This function yields the size of the contents (the pending data) of an i/o
 * buffer.  Use tao_get_buffer_contents() to retrieve the address of the
 * contents of the buffer and tao_adjust_buffer_contents_size() to remove the
 * bytes you may have consumed.

 * @warning This function is meant to be fast.  It assumes its argument is
 * correct and makes no error checking.
 *
 * @param buf    Address of the i/o buffer.
 *
 * @return The number of pending bytes in the buffer.
 *
 * @see tao_get_buffer_contents, tao_adjust_buffer_contents_size.
 */
extern size_t tao_get_buffer_contents_size(tao_buffer_t* buf);

/**
 * Query the contents an i/o buffer.
 *
 * This function yields the address and number of bytes of the contents (the
 * pending bytes) stored in an i/o buffer and which has yet not been consumed.
 * Call tao_adjust_buffer_contents_size() to remove the bytes you may have
 * consumed.
 *
 * @warning The returned information is only valid until no operations that may
 * change the i/o buffer are applied.  The operations which takes a `const
 * tao_buffer_t*` are safe.
 *
 * @warning This function is meant to be fast.  It assumes its arguments are
 * correct and makes no error checking.
 *
 * @param buf    Address of the i/o buffer.
 * @param data   Pointer where to store the address of the first pending byte.
 *               Must not be `NULL`, call tao_get_buffer_contents_size() if you
 *               are only interested in getting the number of pending bytes.
 *
 * @return The number of pending bytes.
 *
 * @see tao_get_buffer_contents_size, tao_adjust_buffer_contents_size.
 */
extern size_t tao_get_buffer_contents(const tao_buffer_t* buf, void** data);

/**
 * Get the size of the unused space in an i/o buffer.
 *
 * This function returns the number of unused bytes after the contents of an
 * i/o buffer.  These bytes are directly available to add more contents to the
 * buffer.  You may call tao_resize_buffer() to make sure enough unused space
 * is available.  Call tao_get_buffer_unused_part() to retrieve the address of
 * the unused part and then, possibly, tao_adjust_buffer_contents_size() to
 * indicate the number of bytes that have been added.

 * @warning This function is meant to be fast.  It assumes its argument is
 * correct and makes no error checking.
 *
 * @param buf    Address of the i/o buffer.
 *
 * @return The number of unused bytes after the contents of the buffer.
 *
 * @see tao_resize_buffer, tao_flush_buffer, tao_get_buffer_unused_part,
 * tao_get_total_unused_buffer_size, tao_adjust_buffer_contents_size.
 */
extern size_t tao_get_buffer_unused_size(const tao_buffer_t* buf);

/**
 * Get the total number of unused bytes in an i/o buffer.
 *
 * This function yields the total number of unused bytes in an i/o buffer.
 * That is, the number of bytes that would be unused after the contents of the
 * buffer if tao_flush_buffer() have been called.
 *
 * @warning This function is meant to be fast.  It assumes its argument is
 * correct and makes no error checking.
 *
 * @param buf    Address of the i/o buffer.
 *
 * @return The total number of unused bytes in the buffer.
 *
 * @see tao_flush_buffer, tao_get_unused_buffer_size.
 */
extern size_t tao_get_total_unused_buffer_size(const tao_buffer_t* buf);

/**
 * Query the unused data at the end of an i/o buffer.
 *
 * This function yields the address and the size of the unused space at the end
 * an i/o buffer and which can directly be used to append more data.  If you
 * write some bytes in the returned space, use
 * tao_adjust_buffer_contents_size() to change the size of the contents of the
 * i/o buffer.
 *
 * @warning The returned information is only valid until no operations that may
 * change the i/o buffer are applied.  The operations which takes a `const
 * tao_buffer_t*` are safe.
 *
 * @warning This function is meant to be fast.  It assumes its arguments are
 * correct and makes on error checking.
 *
 * @param buf    Address of the i/o buffer.
 * @param data   Pointer where to store the address of the first unused byte.
 *               Must not be `NULL`,  call tao_get_buffer_unused_size() if you
 *               are only interested in getting the number of unused bytes.
 *
 * @return The number of unused bytes at the end of the internal data buffer.
 */
extern size_t tao_get_buffer_unused_part(const tao_buffer_t* buf, void** data);

/**
 * Adjust the size of the contents of an i/o buffer.
 *
 * Call this function to pretend that some bytes have been consumed at the
 * beginning of the contents (the pending data) of an i/o buffer or that some
 * bytes have been added to the end of the contents of the an i/o buffer.
 *
 * No more than the number of pending bytes can be consumed and no more than
 * the number of unused bytes after the pending data can be added.  If the
 * adjustment is too large, nothing is done and the function reports a
 * `TAO_OUT_OF_RANGE` error.
 *
 * @param errs   Address of a variable to track errors.
 * @param buf    Address of the i/o buffer.
 * @param adj    Number of bytes to consume (if negative) or add (if positive).
 *
 * @return `0` on success, `-1` on error.
 */
extern int tao_adjust_buffer_contents_size(tao_error_t** errs,
                                           tao_buffer_t* buf, ssize_t adj);

/**
 * Read bytes from a file descriptor to an i/o buffer.
 *
 * This functions attempt to read some bytes from a file descriptor and append
 * them to the contents of an i/o buffer.
 *
 * @param errs   Address of a variable to track errors.
 * @param fd     File descriptor to read from.
 * @param buf    Dynamic buffer to collect bytes read.
 * @param cnt    Number of bytes to read.
 *
 * @return The number of bytes actually read, `-1` in case of errors.  The
 * number of bytes read may be different from @b cnt: it may be smaller if the
 * number of available bytes is insufficient (for instance because we are close
 * to the end of the file or because the peer did not write that much on the
 * other end of the bidirectional communication channel); it may also be larger
 * because, for efficiency reasons, the function attempts to fill the space
 * available in the buffer @b buf.
 *
 * @see tao_write_from_buffer.
 */
extern ssize_t tao_read_to_buffer(tao_error_t** errs, int fd,
                                  tao_buffer_t* buf, size_t cnt);

/**
 * Write the contents of an i/o buffer to a file descriptor.
 *
 * This function attempts to write the contents (the pending data) of an i/o
 * buffer to a file descriptor.  If some bytes are written, they are "removed"
 * from the contents (the pending data) of the i/o buffer.  A single attempt
 * may not be sufficient to write all contents, tao_get_buffer_contents_size()
 * can be used to figure out whether there are remaining unwritten bytes.
 *
 * @param errs   Address of a variable to track errors.
 * @param fd     File descriptor to write to.
 * @param buf    Dynamic buffer whose contents is to be written.
 *
 * @return The number of bytes actually written, `-1` in case of errors.  The
 * returned value may be zero if the contents of the i/o buffer is empty or if
 * the file descriptor is marked for being non-blocking and the operation would
 * block.  To disentangle, call tao_get_buffer_contents_size() to check whether
 * the contents was empty.
 *
 * @see tao_read_to_buffer, tao_get_buffer_contents_size.
 */
extern ssize_t tao_write_from_buffer(tao_error_t** errs, int fd,
                                     tao_buffer_t* buf);

/**
 * Append a formatted message to an i/o buffer.
 *
 * This function appends to the contents of the i/o buffer @b buf a formatted
 * message specified by @b format and subsequent arguments in the same way as
 * the sprintf() function.
 *
 * A final null byte is written in the buffer but is not considered as part of
 * the contents.  So that, if only tao_print_to_buffer(),
 * tao_vprint_to_buffer(), tao_put_string_to_buffer() or
 * tao_put_char_to_buffer() are used to build the contents of the buffer (since
 * creation, initialization or last call to tao_clear_buffer()), the size of
 * the buffer is also the length of the string and, thanks to the final null
 * byte, the buffer contents can be used as a regular string.
 *
 * @param errs   Address of a variable to track errors.
 * @param buf    Dynamic buffer.
 * @param format Format string.
 * @param ...    Subsequent arguments.
 *
 * @return `0` on success, `-1` on error.
 *
 * @see tao_get_buffer_contents(), tao_vprint_to_buffer().
 */
extern int tao_print_to_buffer(tao_error_t** errs, tao_buffer_t* buf,
                               const char* format, ...);

/**
 * Append a formatted message to an i/o buffer.
 *
 * This function is similar to tao_print_to_buffer() except that it is called
 * with a `va_list` instead of a variable number of arguments.
 *
 * @param errs   Address of a variable to track errors.
 * @param buf    Dynamic buffer.
 * @param format Format string.
 * @param ap     Arguments to print.
 *
 * @return `0` on success, `-1` on error.
 *
 * @see tao_get_buffer_contents(), tao_print_to_buffer().
 */
extern int tao_vprint_to_buffer(tao_error_t** errs, tao_buffer_t* buf,
                                const char* format, va_list ap);

/**
 * Append bytes to an i/o buffer.
 *
 * This function appends some bytes to the contents of a dynamic i/o buffer.
 *
 * @param errs   Address of a variable to track errors.
 * @param buf    Dynamic buffer.
 * @param ptr    The address of the first byte to append.
 * @param siz    Number of bytes to append.
 *
 * @return `0` on success, `-1` on error.
 */
extern int tao_append_to_buffer(tao_error_t** errs, tao_buffer_t* buf,
                                const void* ptr, long siz);

/**
 * Append a string to an i/o buffer.
 *
 * This function appends a string to the contents of a dynamic i/o buffer.
 * A final null byte is written in the buffer but is not considered as part of
 * the contents (see tao_print_to_buffer()).
 *
 * @param errs   Address of a variable to track errors.
 * @param buf    Dynamic buffer.
 * @param str    The string to append.
 * @param len    If nonnegative, number of characters to append; otherwise,
 *               the string @a str is assumed null terminated and its length
 *               if givan by calling tao_strlen().
 *
 * @return `0` on success, `-1` on error.
 */
extern int tao_put_string_to_buffer(tao_error_t** errs, tao_buffer_t* buf,
                                    const char* str, long len);

/**
 * Append a single character to an i/o buffer.
 *
 * This function appends a single character to the contents of a dynamic i/o
 * buffer.  A final null byte is written in the buffer but is not considered as
 * part of the contents (see tao_print_to_buffer()).
 *
 * @param errs   Address of a variable to track errors.
 * @param buf    Dynamic buffer.
 * @param c      The character to append.
 *
 * @return `0` on success, `-1` on error.
 */
extern int tao_put_char_to_buffer(tao_error_t** errs, tao_buffer_t* buf,
                                  int c);
/** @} */

/*---------------------------------------------------------------------------*/

/**
 * @addtogroup Arrays
 *
 * Multi-dimensional arrays.
 *
 * Multi-dimensional arrays have homogeneous element type and may have up to
 * @ref TAO_MAX_NDIMS dimensions.  Elements of a multi-dimensional array are
 * contiguous in memory and are stored in
 * [column-major](https://en.wikipedia.org/wiki/Row-_and_column-major_order)
 * order (that is the index along the first dimension varies the fastest).
 *
 * @{
 */

/**
 * Identifier of the type of the elements in an array.
 */
typedef enum tao_element_type {
    TAO_INT8    =   1, /**< Signed 8-bit integer */
    TAO_UINT8   =   2, /**< Unsigned 8-bit integer */
    TAO_INT16   =   3, /**< Signed 16-bit integer */
    TAO_UINT16  =   4, /**< Unsigned 16-bit integer */
    TAO_INT32   =   5, /**< Signed 32-bit integer */
    TAO_UINT32  =   6, /**< Unsigned 32-bit integer */
    TAO_INT64   =   7, /**< Signed 64-bit integer */
    TAO_UINT64  =   8, /**< Unsigned 64-bit integer */
    TAO_FLOAT   =   9, /**< Single precision floating-point */
    TAO_DOUBLE  =  10  /**< Double precision floating-point */
} tao_eltype_t;

/**
 * Get the size of an array element given its type.
 *
 * @param eltype Identifier of the type of the elements of an array.
 *
 * @return A strictly positive number of bytes if @b eltype is valid;
 * `0` is @b eltype is not valid.
 *
 * @see tao_eltype_t.
 */
extern size_t tao_get_element_size(tao_eltype_t eltype);

/**
 * Count the number of elements of a multi-dimensional array.
 *
 * This function counts the number of elements of a multi-dimensional array
 * given its dimensions checking for the validity of the arguments and for
 * integer overflow.
 *
 * @param errs   Address of a variable to track errors.
 * @param ndims  Number of dimensions of the array.
 * @param dims   Lengths of the dimensions of the array.
 *
 * @return The product of the dimensions in @b dims, assuming that a
 *         zer-dimensional array has just one element.  Normally, this value is
 *         at least `1`; `0` is returned in case of errors.
 *
 * @see tao_unreference_array(), tao_wrap_array(), tao_get_array_eltype(),
 *      tao_get_array_length(), tao_get_array_ndims(), tao_get_array_size(),
 *      tao_get_array_data().
 */
extern long tao_count_elements(tao_error_t** errs, int ndims,
                               const long dims[]);

/**
 * Maximun number of dimensions of (shared) arrays.
 */
#define TAO_MAX_NDIMS 5

/**
 * Opaque structure for a multi-dimensional array.
 */
typedef struct tao_array tao_array_t;

/**
 * Create a new array.
 *
 * This function creates a new multi-dimensional array.  The returned array has
 * a reference count of 1, the caller is responsible for unreferencing the
 * array when no longer needed by calling tao_unreference_array().  All the
 * contents of the returned array is stored in a single block of dynamic
 * memory, the first element is stored at an address aligned so as to allow for
 * fast [vectorized
 * operations](https://fr.wikipedia.org/wiki/Single_instruction_multiple_data).
 *
 * @param errs   Address of a variable to track errors.
 * @param eltype Identifier of the type of the elements of the array.
 * @param ndims  Number of dimensions of the array.
 * @param dims   Lengths of the dimensions of the array.
 *
 * @return The address of a new array; `NULL` in case of errors.
 *
 * @see tao_unreference_array(), tao_wrap_array(), tao_get_array_eltype(),
 *      tao_get_array_length(), tao_get_array_ndims(), tao_get_array_size(),
 *      tao_get_array_data().
 */
extern tao_array_t* tao_create_array(tao_error_t** errs, tao_eltype_t eltype,
                                     int ndims, const long dims[]);

/**
 * Create a new mono-dimensional shared array.
 *
 * This function creates a new mono-dimensional array.  The returned array has
 * a reference count of 1, the caller is responsible for unreferencing the
 * array when no longer needed by calling tao_unreference_array().
 *
 * @param errs   Address of a variable to track errors.
 * @param eltype Identifier of the type of the elements of the array.
 * @param dim    Length of the array.
 *
 * @return The address of a new array; `NULL` in case of errors.
 *
 * @see tao_create_array(), tao_unreference_array().
 */
extern tao_array_t* tao_create_1d_array(tao_error_t** errs, tao_eltype_t eltype,
                                        long dim);

/**
 * Create a new two-dimensional array.
 *
 * This function creates a new two-dimensional array.  The returned array has a
 * reference count of 1, the caller is responsible for unreferencing the array
 * when no longer needed by calling tao_unreference_array().
 *
 * @param errs   Address of a variable to track errors.
 * @param eltype Identifier of the type of the elements of the array.
 * @param dim1   Length of the first dimension.
 * @param dim2   Length of the second dimension.
 *
 * @return The address of a new array; `NULL` in case of errors.
 *
 * @see tao_create_array(), tao_unreference_array().
 */
extern tao_array_t* tao_create_2d_array(tao_error_t** errs, tao_eltype_t eltype,
                                        long dim1, long dim2);

/**
 * Create a new three-dimensional array.
 *
 * This function creates a new three-dimensional array.  The returned array has
 * a reference count of 1, the caller is responsible for unreferencing the
 * array when no longer needed by calling tao_unreference_array().
 *
 * @param errs   Address of a variable to track errors.
 * @param eltype Identifier of the type of the elements of the array.
 * @param dim1   Length of the first dimension.
 * @param dim2   Length of the second dimension.
 * @param dim3   Length of the third dimension.
 *
 * @return The address of a new array; `NULL` in case of errors.
 *
 * @see tao_create_array(), tao_unreference_array().
 */
extern tao_array_t* tao_create_3d_array(tao_error_t** errs, tao_eltype_t eltype,
                                        long dim1, long dim2, long dim3);

/**
 * Wrap existing data into a multi-dimensional array.
 *
 * This function creates a multi-dimensional array whose elements are stored in
 * a provided memory area.  The returned array has a reference count of 1, the
 * caller is responsible for unreferencing the array when no longer needed by
 * calling tao_unreference_array().  When the array is eventually destroyed,
 * the callback @b free is called with the context argument @b ctx.
 *
 * @param errs   Address of a variable to track errors.
 * @param eltype Identifier of the type of the elements of the array.
 * @param ndims  Number of dimensions of the array.
 * @param dims   Lengths of the dimensions of the array.
 * @param data   Address of the first element of the array in memory.
 * @param free   Function to call to release the provided resources.
 * @param ctx    Argument of @b free to release the provided resources.
 *
 * @return The address of a new wrapped array; `NULL` in case of errors.
 *
 * @see tao_unreference_array(), tao_create_array(), tao_get_array_eltype(),
 *      tao_get_array_length(), tao_get_array_ndims(), tao_get_array_size(),
 *      tao_get_array_data().
 */
extern tao_array_t* tao_wrap_array(tao_error_t** errs, tao_eltype_t eltype,
                                   int ndims, const long dims[], void* data,
                                   void (*free)(void*), void* ctx);

/**
 * Wrap existing data into a mono-dimensional array.
 *
 * This function creates a mono-dimensional array whose elements are stored in
 * a provided memory area.  The returned array has a reference count of 1, the
 * caller is responsible for unreferencing the array when no longer needed by
 * calling tao_unreference_array().  When the array is eventually destroyed,
 * the callback @b free is called with the context argument @b ctx.
 *
 * @param errs   Address of a variable to track errors.
 * @param eltype Identifier of the type of the elements of the array.
 * @param dim    Length of the mono-dimensional array.
 * @param data   Address of the first element of the array in memory.
 * @param free   Function to call to release the provided resources.
 * @param ctx    Argument of @b free to release the provided resources.
 *
 * @return The address of a new wrapped array; `NULL` in case of errors.
 *
 * @see tao_wrap_array(), tao_unreference_array().
 */
extern tao_array_t* tao_wrap_1d_array(tao_error_t** errs, tao_eltype_t eltype,
                                      long dim, void* data,
                                      void (*free)(void*), void* ctx);

/**
 * Wrap existing data into a two-dimensional array.
 *
 * This function creates a two-dimensional array whose elements are stored in a
 * provided memory area.  The returned array has a reference count of 1, the
 * caller is responsible for unreferencing the array when no longer needed by
 * calling tao_unreference_array().  When the array is eventually destroyed,
 * the callback @b free is called with the context argument @b ctx.
 *
 * @param errs   Address of a variable to track errors.
 * @param eltype Identifier of the type of the elements of the array.
 * @param dim1   Length of the first dimension.
 * @param dim2   Length of the second dimension.
 * @param data   Address of the first element of the array in memory.
 * @param free   Function to call to release the provided resources.
 * @param ctx    Argument of @b free to release the provided resources.
 *
 * @return The address of a new wrapped array; `NULL` in case of errors.
 *
 * @see tao_wrap_array(), tao_unreference_array().
 */
extern tao_array_t* tao_wrap_2d_array(tao_error_t** errs, tao_eltype_t eltype,
                                      long dim1, long dim2, void* data,
                                      void (*free)(void*), void* ctx);

/**
 * Wrap existing data into a three-dimensional array.
 *
 * This function creates a three-dimensional array whose elements are stored in
 * a provided memory area.  The returned array has a reference count of 1, the
 * caller is responsible for unreferencing the array when no longer needed by
 * calling tao_unreference_array().  When the array is eventually destroyed,
 * the callback @b free is called with the context argument @b ctx.
 *
 * @param errs   Address of a variable to track errors.
 * @param eltype Identifier of the type of the elements of the array.
 * @param dim1   Length of the first dimension.
 * @param dim2   Length of the second dimension.
 * @param dim3   Length of the third dimension.
 * @param data   Address of the first element of the array in memory.
 * @param free   Function to call to release the provided resources.
 * @param ctx    Argument of @b free to release the provided resources.
 *
 * @return The address of a new wrapped array; `NULL` in case of errors.
 *
 * @see tao_wrap_array(), tao_unreference_array().
 */
extern tao_array_t* tao_wrap_3d_array(tao_error_t** errs, tao_eltype_t eltype,
                                      long dim1, long dim2, long dim3,
                                      void* data, void (*free)(void*),
                                      void* ctx);

/**
 * Add a reference to an existing multi-dimensional array.
 *
 * This function increments the reference count of a multi-dimensional array
 * by one.  The caller is responsible for eventually unreferencing the array by
 * calling tao_unreference_array().
 *
 * @param arr Pointer to an array (must not be `NULL`).
 *
 * @return The address of the array @b arr.
 *
 * @see tao_create_array(), tao_wrap_array(), tao_unreference_array().
 */
extern tao_array_t* tao_reference_array(tao_array_t* arr);

/**
 * Drop a reference from a multi-dimensional array.
 *
 * This function decrements the reference count of a multi-dimensional array by
 * one.  When the reference count reach the value of zero the multi-dimensional
 * array is effectively destroyed.
 *
 * @param arr    Pointer to an array referenced by the caller.
 *
 * @see tao_create_array(), tao_wrap_array(), tao_reference_array().
 */
extern void tao_unreference_array(tao_array_t* arr);

/**
 * Get the type of elements of an array.
 *
 * @param arr    Pointer to an array referenced by the caller.
 *
 * @return One of the `tao_eltype_t` values.
 */
extern tao_eltype_t tao_get_array_eltype(const tao_array_t* arr);

/**
 * Get the number of elements of an array.
 *
 * @param arr    Pointer to an array referenced by the caller.
 *
 * @return The number of elements in the array.
 */
extern long tao_get_array_length(const tao_array_t* arr);

/**
 * Get the number of dimensions of an array.
 *
 * @param arr    Pointer to an array referenced by the caller.
 *
 * @return The number of dimensions of the array.
 */
extern int tao_get_array_ndims(const tao_array_t* arr);

/**
 * Get the length of a dimension of an array.
 *
 * All dimensions beyond the number of dimensions of @b arr are assumed to have
 * unit length.
 *
 * @param arr    Pointer to an array referenced by the caller.
 * @param d      Index of dimension of interest (`1` is the first dimension).
 *
 * @return The number of elements along the given dimension if @b d is greater
 *         or equal `1` and less or equal the number of dimensions of @b arr;
 *         `0` if @b d is less than `1` and `1` if @b d is greater than the
 *         number of dimensions of @b arr.
 */
extern long tao_get_array_size(const tao_array_t* arr, int d);

/**
 * Get the address of the first element of an array.
 *
 * Elements of a multi-dimensional array are contiguous in memory and are
 * stored in
 * [column-major](https://en.wikipedia.org/wiki/Row-_and_column-major_order)
 * order (that is the index along the first dimension varies the fastest).
 *
 * @param arr    Pointer to an array referenced by the caller.
 *
 * @return The address of the first element of the array.
 */
extern void* tao_get_array_data(const tao_array_t* arr);

/** @} */

/*---------------------------------------------------------------------------*/

/**
 * @addtogroup Utilities
 *
 * Low-level and utilities functions.
 *
 * @{
 */

/**
 * Get the length of a string.
 *
 * This function behaves as strlen() except that a `NULL` argument yield 0.
 *
 * @param str   String.
 *
 * @return The length of the string; 0 if @b str is `NULL`.
 *
 * @see strlen.
 */
extern size_t tao_strlen(const char* str);

/**
 * @addtogroup Commands
 *
 * Parsing of commands.
 *
 * These functions are provided to split lines of commands into arguments and
 * conversely to pack arguments into a command-line.
 *
 * Just like the `argv` array in C `main` function, a command is a list of
 * "words" which are ordinary C-strings.  All characters are allowed in a word
 * except the null character `\0` as it serves as a marker of the end of the
 * string.
 *
 * In order to communicate, processes may exchange commands packed into a
 * single string and sent through communication channels.  In order to allow
 * for sendind/receiving several successive commands and for coping with
 * partially transmitted commands, their size must be part of the sent data or
 * they must be terminated by some given marker.  In order to make things
 * simple, it has been chosen that successive commands be separated by a single
 * line-feed character (`'\n'`, ASCII code `0x0A`).  This also simplify the
 * writing of commands into scripts.
 *
 * String commands have to be parsed into words before being used.  Since any
 * character (but the null) is allowed in such words there must be means to
 * separate words in command strings and to allow for having a line-feed
 * character in a word (not a true one because it is used to indicate end of
 * the command string).
 *
 * The following rules are applied to parse a command string into words:
 *
 * 1- An end-of-line (EOL) sequence at the end of the command string is allowed
 *    and ignored.  To cope with different styles, an EOL can be any of the
 *    following sequences: a single carriage-return (CR, ASCII code `0x0D`)
 *    character, a single line-feed (LF, ASCII code `0x0A`) character or a
 *    sequence of the two characters CR-LF.
 *
 * 2- Leading and trailing spaces in a command string are ignored (trailing
 *    spaces may occur before the EOL sequence if any but not after).  Space
 *    characters are ordinary spaces `' '` (ASCII code `0x20`) or tabulations
 *    `'\t'` (ASCII code `0x09`).
 *
 * 3- Words are separated by one or more space characters.  A word is either a
 *    sequence of contiguous ordinary characters (non-space, non-quote,
 *    non-escape, non-forbidden) or a quoted string (see next).
 *
 * 6- Strings start with a quoting character (either a single or a double
 *    quote) and end with the same quoting character.  The opening and closing
 *    quotes are not part of the resulting word.  There must be at least one
 *    space character after (respectively before) the openning (respectively
 *    closing) quote if the string is not the first (respectively last) word of
 *    the sequence.  That is, quotes cannot be part of non-quoted words and are
 *    therefore not considered as ordinary characters.  There are 2 forms of
 *    quoted strings: strings enclosed in single quotes are extracted literaly
 *    (there may be double quotes inside but no single quotes and the escape
 *    character is considered as an ordinary character in literal strings) and
 *    strings enclosed in double quotes which may contain escape sequence to
 *    represent some special characters.  The following escape sequences are
 *    allowed and recognized in double quoted strings (any other occurences of
 *    the escape character is considered as an error):
 *
 *   - `\t` yields an horizontal tabulation character;
 *   - `\n` yields a line-feed character;
 *   - `\r` yields a carriage-return character;
 *   - `\"` yields a double-quote character;
 *   - `\\` yields a backslash character.
 *
 * Thus quoted strings can have embedded spaces.  To have a zero-length word, a
 * quoted string like `''` or `""` must be used.
 *
 * The following errors may occur:
 *
 *  - Illegal characters: Anywhere but at the end of the command string, CR and
 *    LF characters are considered as an error.  This is because LF are used to
 *    separate successive commands in communication channels.  The null
 *    character must not appear in the command string (as it serves as end of
 *    string marker).
 *
 *  - Successive quoted strings not separated by a space.  Quotes appearing in
 *    a non-quoted word.  Unclosed quoted strings.  Unterminated escape
 *    sequences.
 *
 * @{
 */

/**
 * Split command in individual words.
 *
 * @param errs   Address of a variable to track errors.
 * @param list   Address of a variable to store the list.  The list and its
 *               constitutive words are stored in a single block of memory.
 * @param cmd    Command line string to split.
 * @param len    Optional length of @b cmd.  If @b len is nonnegative, it is
 *               assumed to give the number of characters of the command line
 *               (which must not then be null-terminated).  Otherwise, @b len
 *               may be equal to `-1` and the command line must be
 *               null-terminated.
 *
 * The caller is responsible for properly initializing the list pointer to
 * `NULL` and calling free() when the list is no longer needed.  The list
 * argument can be re-used several times to parse different command lines.
 *
 * @fixme What are the advantages of this kind of allocation?  This is only
 * useful if we can avoid reallocating such lists.  There is no portable way to
 * query the size of a malloc'ed block of memory given its address.  The only
 * possibility is to make a hack an allocate more bytes to store the size
 * (however beware of alignment).  On my laptop, malloc() and free() of blocks
 * or random sizes (between 1 and 10,000 bytes) takes less than 100ns on
 * average.
 *
 * @return The number of words in the list; `-1` in case of errors.
 */
extern int tao_split_command(tao_error_t** errs, const char*** list,
                             const char* cmd, long len);

/**
 * Pack words into a command-line.
 *
 * This function assembles given words into a single command-line.  This
 * function does the opposite of tao_unpack_words().
 *
 * @param errs   Address of a variable to track errors.
 * @param dest   Address of an i/o buffer to store the result.  The resulting
 *               command-line is appended to any existing contents of @b dest.
 * @param argv   List of words.  The elements of the list must be non-null
 *               pointers to ordinary C-strings terminated by a null character.
 *               If @b argc is equal to `-1`, the list must have one more
 *               pointer then the number of words, this last pointer being set
 *               to `NULL` to mark the end of the list.
 * @param argc   Optional number of words in @b argv.  If @b argc is
 *               nonnegative, it is assumed to give the number of words in the
 *               list.  Otherwise, @b argc can be equal to `-1` to indicate
 *               that the first null-pointer in @b argv marks the end of the
 *               list.
 *
 * @return `0` on success; `-1` on failure.
 *
 * @note In case of failure, the contents of @b dest existing prior to the call
 * is untouched but its location may have change.
 *
 * @see tao_unpack_words.
 */
extern int tao_pack_words(tao_error_t** errs, tao_buffer_t* dest,
                          const char* argv[], int argc);

/**
 * Read an `int` value in a word.
 *
 * This function is intended to parse an integer value from a word as obtained
 * by splitting commands with tao_split_command().  To be valid, the word must
 * contains single integer in a human redable form and which fits in an `int`.
 *
 * @param str    Input word to parse.
 * @param ptr    Address to write parsed value.
 * @param base   Base to use for conversion, `0` to apply C-style conventions.
 *
 * @return `0` on success, `-1` on error.  In case of error, the value stored
 *         at `*ptr` is left unchanged.
 */
extern int tao_parse_int(const char* str, int* ptr, int base);

/**
 * Read a `long` value in a word.
 *
 * This function is intended to parse an integer value from a word as obtained
 * by splitting commands with tao_split_command().  To be valid, the word must
 * contains single integer in a human redable form and which fits in a `long`.
 *
 * @param str    Input word to parse.
 * @param ptr    Address to write parsed value.
 * @param base   Base to use for conversion, `0` to apply C-style conventions.
 *
 * @return `0` on success, `-1` on error.  In case of error, the value stored
 *         at `*ptr` is left unchanged.
 */
extern int tao_parse_long(const char* str, long* ptr, int base);

/**
 * Read a `double` value in a word.
 *
 * This function is intended to parse a floating point value from a word as
 * obtained by splitting commands with tao_split_command().  To be valid, the
 * word must contains single floating value in a human redable form.
 *
 * @param str    Input word to parse.
 * @param ptr    Address to write parsed value.
 *
 * @return `0` on success, `-1` on error.  In case of error, the value stored
 *         at `*ptr` is left unchanged.
 */
extern int tao_parse_double(const char* str, double* ptr);

/** @} */

/**
 * @addtogroup DynamicMemory
 *
 * Management of dynamic memory.
 *
 * These functions are provided to report errors like other fucntions in AO
 * library.
 *
 * @{
 */

/**
 * Allocate dynamic memory.
 *
 * This function behaves as malloc() except that error information may be
 * tracked in @b errs.  The caller is responsible for calling free() or
 * tao_free() to free the allocated memory when no longer needed.
 *
 * @param errs   Address of a variable to track errors.
 * @param size   Number of bytes to allocate.
 *
 * @return The address of allocated dynamic memory; `NULL` in case of errors.
 *
 * @see tao_free, tao_calloc.
 */
extern void* tao_malloc(tao_error_t** errs, size_t size);

/**
 * Allocate dynamic memory.
 *
 * This function behaves as calloc() except that error information may be
 * tracked in @b errs.  The caller is responsible for calling free() or
 * tao_free() to free the allocated memory when no longer needed.
 *
 * @param errs   Address of a variable to track errors.
 * @param nelem  Number of elements to allocate.
 * @param elsize Number of bytes per element.
 *
 * @return The address of allocated dynamic memory; `NULL` in case of errors.
 *
 * @see tao_free, tao_malloc.
 */
extern void* tao_calloc(tao_error_t** errs, size_t nelem, size_t elsize);

/**
 * Free dynamic memory.
 *
 * This function behaves as free() except that it accepts a `NULL` pointer.
 *
 * @param ptr    Address of dynamic memory (can be `NULL`).
 *
 * @see tao_malloc, tao_calloc.
 */
extern void tao_free(void* ptr);

/** @} */

/**
 * @addtogroup Time
 *
 * Measurement of time and time intervals.
 *
 * Time is stored in a structure of type `struct timespec` which is defined in
 * `<time.h>` and which has a nanosecond resolution.  The actual precision
 * however depends on the resolution of the functions provided by the system to
 * get a time.  The maximum time amplitude that can be repesented is
 * @f$\approx\pm2.9\times10^{11}@f$ years (nearly 20 times the age of the
 * Universe).  So it is probably sufficient to represent any absolute time.
 *
 * The function tao_get_monotonic_time() can be used to precisely measure time
 * intervals, while the function tao_get_current_time() can be called to get
 * the current time.
 *
 * @{
 */

/**
 * One nanosecond in SI units (seconds).
 */
#define TAO_NANOSECOND (1e-9*TAO_SECOND)

/**
 * One microsecond in SI units (seconds).
 */
#define TAO_MICROSECOND (1e-6*TAO_SECOND)

/**
 * One microsecond in SI units (seconds).
 */
#define TAO_MILLISECOND (1e-3*TAO_SECOND)

/**
 * One second in SI units (seconds).
 */
#define TAO_SECOND (1.0)

/**
 * One minute in SI units (seconds).
 */
#define TAO_MINUTE (60*TAO_SECOND)

/**
 * One hour in SI units (seconds).
 */
#define TAO_HOUR (60*TAO_MINUTE)

/**
 * One day in SI units (seconds).
 */
#define TAO_DAY (24*TAO_HOUR)

/**
 * One year in SI units (seconds).
 */
#define TAO_YEAR (365.25*TAO_DAY)

/**
 * Sleep for a specified hight-resolution number of seconds.
 *
 * This function causes the calling thread to sleep either until the number of
 * specified seconds have elapsed or until a signal arrives which is not
 * ignored.
 *
 * @param secs   The amount of time to sleep in seconds.  Must be nonnegative
 *               and less than the maximum value of a `time_t` integer which is
 *               is fairly large (at least about 68 years).  Can have a high
 *               resolution, at least one microsecond but, depepending on the
 *               operationg system, it may be as good as one nanosecond.
 *
 * @return `0` on success, `-1` on error or in case of interrupt (check the
 *         value of `errno` to figure out the reason of the failure).
 */
extern int tao_sleep(double secs);

/**
 * Get monotonic time.
 *
 * This function yields a monotonic time since some unspecified starting point
 * but which is not affected by discontinuous jumps in the system time (e.g.,
 * if the system administrator manually changes the clock), but is affected by
 * the incremental adjustments performed by adjtime() and NTP.
 *
 * @warning For systems where it is not possible to retrieve a monotonic time,
 *          the time given by tao_get_current_time() is returned instead.
 *
 * @param errs   Address of a variable to track errors.
 * @param dest   Address to store the time.
 *
 * @return `0` on success, `-1` on error.  In case of error, 0 seconds and 0
 * nanoseconds are stored in @b dest.
 */
extern int tao_get_monotonic_time(tao_error_t** errs, struct timespec* dest);

/**
 * Get the current time.
 *
 * This function yields the current time since the
 * [Epoch](https://en.wikipedia.org/wiki/Unix_time), that is 00:00:00 UTC, 1
 * January 1970.
 *
 * @param errs   Address of a variable to track errors.
 * @param dest   Address to store the time.
 *
 * @return `0` on success, `-1` on error.  In case of error, 0 seconds and 0
 * nanoseconds are stored in @b dest.
 */
extern int tao_get_current_time(tao_error_t** errs, struct timespec* dest);

/**
 * Normalize time.
 *
 * This function adjusts the members of a `timespec` structure so that the time
 * stored in the structure is correct.  More specifically it ensures that the
 * number of nanoseconds is in the range 0 to 999,999,999.
 *
 * @param ts   Address of a `timespec` structure to adjust.
 *
 * @return The address @a ts.
 */
extern struct timespec* tao_normalize_time(struct timespec* ts);

/**
 * Add times.
 *
 * This function adds 2 times.
 *
 * @warning This function is meant to be fast.  It makes no checking about the
 * validity of the arguments nor integer oveflows.  Normally the destination
 * time is such that the number of nanoseconds is nonnegative and strictly less
 * than 1,000,000,000.
 *
 * @param dest   Address to store the result.
 * @param a      Address of first time value.
 * @param b      Address of second time value.
 *
 * @return The address @a dest.
 */
extern struct timespec* tao_add_times(struct timespec* dest,
                                      const struct timespec* a,
                                      const struct timespec* b);

/**
 * Subtract times.
 *
 * This function subtracts 2 times.
 *
 * @warning This function is meant to be fast.  It makes no checking about the
 * validity of the arguments nor integer oveflows.  Normally the destination
 * time is such that the number of nanoseconds is nonnegative and strictly less
 * than 1,000,000,000.
 *
 * @param dest   Address to store the result.
 * @param a      Address of first time value.
 * @param b      Address of second time value.
 *
 * @return The address @a dest.
 */
extern struct timespec* tao_subtract_times(struct timespec* dest,
                                           const struct timespec* a,
                                           const struct timespec* b);

/**
 * Convert time in seconds.
 *
 * @param t      Address of time value.
 *
 * @return The number of seconds given by the time stored in @b t.
 */
extern double tao_time_to_seconds(const struct timespec* t);

/**
 * Convert a number of seconds into a time structure.
 *
 * @param dest   Address to store the result.
 * @param secs   A fractional number of seconds.
 *
 * @return The address @b dest.
 *
 * @warning This function never fails.  If @b secs is too large (in amplitude)
 * to be represented, `INT64_MAX` or `INT64_MIN` seconds and 0 nanoseconds are
 * assumed.  If @b secs is a NaN (Not a Number), 0 seconds and -1 nanoseconds
 * are assumed.  Otherwise, the number of seconds stored in @b dest is strictly
 * greater than `INT64_MIN` and strictly less than `INT64_MAX` while the number
 * of nanoseconds is greater or equal 0 and strictly less than 1,000,000,000.
 * It is therefore always possible guess from the stored time whether @b secs
 * was representable as a time structure with nanosecond precision.
 */
extern struct timespec* tao_seconds_to_time(struct timespec* dest,
                                            double secs);

/**
 * Print a time-stamp in a human readable form to a string.
 *
 * @param str    Destination string (must have at least 32 characters
 *               including the terminating null).
 * @param ts     Time stamp.
 *
 * @return The address @b str.
 */
extern char* tao_sprintf_time(char* str, const struct timespec* ts);

/**
 * Print a time-stamp in a human readable form to a string.
 *
 * If the destination @a str is non-`NULL`, this function writes as much as
 * possible of the resulting string in @a str but no more than `size` bytes and
 * with a terminating null.  In any cases, the length of the string
 * corresponding to the complete result (excluding the terminating null) is
 * returned.
 *
 * @param str    Destination string (nothing is written there if `NULL`).
 * @param size   Number of bytes available in the destination string.
 * @param ts     Time stamp.
 *
 * @return The number of bytes needed to store the complete formatted string
 * (excluding the terminating null).  Thus, a return value of @a size or more
 * means that the output was truncated.
 */
extern size_t tao_snprintf_time(char* str, size_t size,
                                const struct timespec* ts);

/**
 * Print a time-stamp in a human readable form to a file stream.
 *
 * @param stream Destination stream.
 * @param ts     Time stamp.
 */
extern void tao_fprintf_time(FILE* stream, const struct timespec* ts);

/**
 * Compute absolute timeout.
 *
 * This function computes an absolute timeout given a duration relative to the
 * current time (as given by the clock `CLOCK_REALTIME`).  The function tries
 * to avoid overflows (integers would wrap with negative values in that case)
 * and stores the maximum possible value in @a ts.  Call
 * tao_is_finite_timeout() to check whether that happens.
 *
 * @param errs   Address of a variable to track errors.
 * @param tm     Address of `struct timespec` structure.
 * @param secs   Number of seconds from now.
 *
 * @return `0` on success, `-1` on failure.
 */
extern int tao_get_absolute_timeout(tao_error_t** errs,
                                    struct timespec* tm, double secs);

/**
 * Check whether the absolute time is finite.
 *
 * This function checks whether the time since the Epoch (absolute time) stored
 * in @a ts is strictly less than the maximum possible value.
 *
 * @param tm     Address of `struct timespec` structure.
 *
 * @return A boolean value.
 */
extern bool tao_is_finite_absolute_time(struct timespec* tm);

/**
 * Maximum number of seconds since the Epoch.
 *
 * This function yields the maximum number of seconds that fit in a `time_t`
 * integer.  Type `time_t` is at least 32-bit signed integer which gives a
 * maximum of 2.147483647e9 seconds (more than 68 years) since the Epoch.  On
 * most modern systems, `time_t` is 64-bit signed integer which gives a maximum
 * of about 9.223372036854776e18 seconds (more than 2.9e11 years) since the
 * Epoch.  Note that ISO-C standard only specifies that `time_t` should be a
 * real, but I've never seen a system where it was not a signed integer.
 *
 * @return A large number of seconds.
 */
extern double tao_get_maximum_absolute_time();

/** @} */

/**
 * @addtogroup Locks
 *
 * Mutexes, conditions variables and semaphores.
 *
 * @{
 */

/**
 * Initialize a non-static mutex.
 *
 * This functions initialize a mutex prior to its usage.  The function
 * tao_destroy_mutex() must be called when the mutex is no longer needed.
 *
 * @param errs   Address of a variable to track errors.
 * @param mutex  Pointer to the mutex to initialize.
 * @param shared If non-zero, require that the mutex be accessible between
 *               processes; otherwise, the mutex will be *private* (that is,
 *               only accessible by threads in the same process as the caller).
 *
 * @return `0` if successful; `-1` in case of error.
 */
extern int tao_initialize_mutex(tao_error_t** errs, pthread_mutex_t* mutex,
                                bool shared);

/**
 * Lock a mutex.
 *
 * @param errs   Address of a variable to track errors.
 * @param mutex  Pointer to the mutex to lock.
 *
 * @return `0` if successful; `-1` in case of error.
 */
extern int tao_lock_mutex(tao_error_t** errs, pthread_mutex_t* mutex);

/**
 * Try to lock a mutex.
 *
 * @param errs   Address of a variable to track errors.
 * @param mutex  Pointer to the mutex to lock.
 *
 * @return `1` if mutex has been locked by the caller; `0` if the mutex is
 * already locked by some other thread/process; `-1` in case of error.
 */
extern int tao_try_lock_mutex(tao_error_t** errs, pthread_mutex_t* mutex);

/**
 * Unlock a mutex.
 *
 * @param errs   Address of a variable to track errors.
 * @param mutex  Pointer to the mutex to unlock.
 *
 * @return `0` if successful; `-1` in case of error.
 */
extern int tao_unlock_mutex(tao_error_t** errs, pthread_mutex_t* mutex);

/**
 * Destroy a mutex.
 *
 * This function destroys a mutex that has been initialized by
 * tao_initialize_mutex().
 *
 * @param errs   Address of a variable to track errors.
 * @param mutex  Pointer to the mutex to destroy.
 * @param wait   If the mutex is locked, this parameter specifies whether the
 *               function should block until the mutex is unlocked by its
 *               owner.
 *
 * @return `0` if successful; `-1` in case of error.
 */
extern int tao_destroy_mutex(tao_error_t** errs, pthread_mutex_t* mutex,
                             bool wait);

/**
 * Initialize a condition variable.
 *
 * This function initializes a non-static condition variable.  The caller is
 * responsible of calling tao_destroy_condition() to free the resources that
 * may be associated with the condition variable.
 *
 * @param errs   Address of a variable to track errors.
 * @param cond   Pointer to the condition variable to initialize.
 *
 * @return `0` if successful; `-1` in case of error.
 */
extern int tao_initialize_condition(tao_error_t** errs, pthread_cond_t* cond);

/**
 * Destroy a condition variable.
 *
 * This function destroys a condition variable that has been initialized by
 * tao_initialize_condition().
 *
 * @param errs   Address of a variable to track errors.
 * @param cond   Pointer to the condition variable to destroy.
 *
 * @return `0` if successful; `-1` in case of error.
 */
extern int tao_destroy_condition(tao_error_t** errs, pthread_cond_t* cond);

/**
 * Signal a condition variable to at most one thread.
 *
 * This function restarts one of the threads that are waiting on the condition
 * variable @b cond.  Nothing happens, if no threads are waiting on the
 * condition variable.
 *
 * @param errs   Address of a variable to track errors.
 * @param cond   Pointer to the condition variable to signal.
 *
 * @return `0` if successful; `-1` in case of error.
 */
extern int tao_signal_condition(tao_error_t** errs, pthread_cond_t* cond);

/**
 * Signal a condition variable to all waiting thread.
 *
 * This function behaves like tao_signal_condition() except that all threads
 * waiting on the condition variable @a cond are restarted.  Nothing happens,
 * if no threads are waiting on the condition variable.
 *
 * @param errs   Address of a variable to track errors.
 * @param cond   Pointer to the condition variable to signal.
 *
 * @return `0` if successful; `-1` in case of error.
 */
extern int tao_broadcast_condition(tao_error_t** errs, pthread_cond_t* cond);

/**
 * Wait for a condition to be signaled.
 *
 * This function atomically unlocks the mutex and waits for the condition
 * variable to be signaled.  The thread execution is suspended and does not
 * consume any CPU time until the condition variable is signaled. The mutex
 * must be locked by the calling thread on entrance to this function.  Before
 * returning to the calling thread, this function re-acquires the mutex.
 *
 * @param errs   Address of a variable to track errors.
 * @param cond   Address of the condition variable to wait on.
 * @param mutex  Address of the mutex associated with the condition variable.
 *
 * @return `0` on success, `-1` in case of error.
 *
 * @see tao_wait_condition, tao_get_absolute_timeout.
 */
extern int tao_wait_condition(tao_error_t** errs, pthread_cond_t* cond,
                              pthread_mutex_t* mutex);

/**
 * Attempt to wait for a condition to be signaled.
 *
 * This function behaves like tao_wait_condition() but blocks no longer than
 * some given duration.
 *
 * @param errs   Address of a variable to track errors.
 * @param cond   Address of the condition variable to wait on.
 * @param mutex  Address of the mutex associated with the condition variable.
 * @param secs   Maximum time to wait (in seconds).  If this amount of time is
 *               very large (more than one year), the effect is the same as
 *               calling tao_wait_condition().
 *
 * @return `1` if the codition is signaled before the specified number of
 * seconds; `0` if timeout occured before; `-1` in case of error.
 *
 * @see tao_wait_condition, tao_get_absolute_timeout.
 */
extern int tao_timed_wait_condition(tao_error_t** errs, pthread_cond_t* cond,
                                    pthread_mutex_t* mutex, double secs);

/** @} */

/**
 * @addtogroup Macros
 *
 * Useful macros.
 *
 * @{
 */

/*
 * Helpers for branch prediction.  See
 * http://blog.man7.org/2012/10/how-much-do-builtinexpect-likely-and.html
 * and https://stackoverflow.com/questions/109710/how-does-the-likely-unlikely-macros-in-the-linux-kernel-works-and-what-is-their.
 */

#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#  define TAO_LIKELY(expr)      (__builtin_expect(!!(expr), 1))
#  define TAO_UNLIKELY(expr)    (__builtin_expect(!!(expr), 0))
#else
#  define TAO_LIKELY(expr)      (expr)
#  define TAO_UNLIKELY(expr)    (expr)
#endif

/**
 * @def TAO_LIKELY
 *
 * @brief Indicate to the compiler that an expression is expected to be true.
 *
 * The @ref TAO_LIKELY and @ref TAO_UNLIKELY macros let the programmer give
 * hints to the compiler about the expected result of an expression.  Some
 * compilers can use this information for optimizations.
 *
 * @param expr   The expression to test.
 *
 * @see TAO_UNLIKELY.
 */

/**
 * @def TAO_UNLIKELY
 *
 * @brief Indicate to the compiler that an expression is expected to be false.
 *
 * @param expr   The expression to test.
 *
 * @see TAO_LIKELY.
 */

/** @} */

/** @} */

/*---------------------------------------------------------------------------*/

/**
 * @addtogroup SharedObjects
 *
 * Objects whose contents is shared between processes.
 *
 * @{
 */

/**
 * @addtogroup BasicSharedObjects
 *
 * Basic building block for shared objects.
 *
 * @{
 */

#define TAO_SHARED_MAGIC     0x310efc00
#define TAO_SHARED_MASK      0xffffff00
#define TAO_SHARED_MAX_SIZE  0xffffffff
#define TAO_SHARED_MIN_SIZE  sizeof(tao_shared_object_t)

/** Type identifiers of shared objects. */
typedef enum tao_object_type {
    TAO_SHARED_OBJECT = (TAO_SHARED_MAGIC | 0), /**< Basic shared object */
    TAO_SHARED_ARRAY  = (TAO_SHARED_MAGIC | 1), /**< Shared multi-dimensional
                                                 *   array */
    TAO_SHARED_CAMERA = (TAO_SHARED_MAGIC | 2), /**< Frame grabber shared
                                                 *   data */
    TAO_SHARED_ANY    = 0xffffffff              /**< Any shared object */
} tao_object_type_t;

/**
 * Shared objects.
 *
 * Structure `tao_shared_object_t` defines the common part of all shared object
 * types and it is assumed that a reference to a shared object of any type
 * derived from `tao_shared_object_t` can be safely cast as a
 * `tao_shared_object_t*`.  In addition, shared objects are stored in a single
 * segment of shared memory.  Hence memory beyond `sizeof(tao_shared_object_t)`
 * is used to store other members and data (with suitable alignment).
 *
 * The structure defining a specific shared object type derived from
 * `tao_shared_object_t` should then be defined by something like:
 *
 * ```.c
 * struct some_shared_object_type {
 *     tao_shared_object_t base; // Base of any TAO shared object
 *     some_type some_member;    // First member specific to the derived type
 *     some_other_type some_other_member; // etc.
 * }
 * ```
 *
 * In shared memory, the object is stored as:
 *
 * ```
 * basic object members
 * specific members
 * padding
 * object data
 * ```
 *
 * where `padding` represent any amount of unused bytes required for proper
 * alignment of the `data` part.
 *
 * - The member `mutex` is used to monitor the usage of the object.  It must be
 *   initialized so as to be usable by different processes (not just different
 *   threads).
 *
 * - The member `nrefs` counts the total number of references to the object.
 *   Once the number of references becomes equal to zero, the object is assumed
 *   to be no longer in use and is available for being destroyed.  A shared
 *   object is created with a single reference (*i.e.* the creator of the
 *   object is assumed to have a reference on the new object).
 *
 * - The member `type` is an integer which uniquely identify the type (or class)
 *   of the object.
 *
 * - The member `size` is the total number of bytes used to store the object
 *   contents.
 *
 * - The member `ident` is an integer which uniquely identify the object.
 *
 * Members `type`, `size` and `ident` are constants that must be considered as
 * read-only during the life time of the object.  It is therefore safe to read
 * them without locking the object (providing the reader has a reference on the
 * object to prevent that it is destroyed while reading these members).
 */
typedef struct tao_shared_object {
    pthread_mutex_t mutex;  /**< Mutex associated with the object */
    int32_t         nrefs;  /**< Number of references on the object */
    int32_t         ident;  /**< Unique identifier of the object */
    size_t          size;   /**< Total number of bytes allocated for the
                             *   object */
    int32_t         type;   /**< Object type */
} tao_shared_object_t;

/**
 * Create a new shared object.
 *
 * This function creates a new shared object of given type and size.  The
 * object is stored in shared memory so that it can be accessible by other
 * processes (calling tao_attach_shared_object()).  The new object is initially
 * attached to the address space of the caller which is assumed to have a
 * reference on the object.  Hence, the returned object has a reference count
 * of one.  When the object is no longer used by the caller, the caller must
 * call tao_detach_shared_object() to detach the object from its address space
 * and decrement its reference count by one.
 *
 * The remaining bytes after the basic object information are all set to zero.
 *
 * @param errs   Address of a variable to track errors.
 * @param type   Type identifier of the object.
 * @param size   Total number of bytes to allocate.
 * @param perms  Permissions granted to the group and to the others.  At least,
 *               read and write access (that is bits `S_IRUSR` and `S_IWUSR`)
 *               are granted for the caller.
 *
 * @return The address of the new object in the address space of the caller;
 * `NULL` on failure.
 */
extern tao_shared_object_t* tao_create_shared_object(tao_error_t** errs,
                                                     tao_object_type_t type,
                                                     size_t size,
                                                     unsigned int perms);

/**
 * Attach an existing shared object to the address space of the caller.
 *
 * This function attaches an existing shared object to the address space of the
 * caller.  The caller will have a reference on the returned object (whose
 * reference count of the shared object is therefore incremented by one).  When
 * the object is no longer used by the caller, the caller must call
 * tao_detach_shared_object() to detach the object from its address space,
 * decrement its reference count by one and eventually free the shared memory
 * associated with the array.
 *
 * The same process may attach a shared object more than once but each
 * attachment, due to tao_attach_shared_object() or to
 * tao_create_shared_object(), should be matched by a
 * tao_detach_shared_object() with the corresponding address in the caller's
 * address space.
 *
 * @param errs   Address of a variable to track errors.
 * @param ident  Unique identifier of the shared object.
 * @param type   Expected type identifier of the shared object.  Can be
 *               `TAO_SHARED_ANY` to accept any shared object type.
 *
 * @return The address of the shared object in the address space of the caller;
 * `NULL` on failure.  Even tough the arguments are correct, an error may arise
 * if the object has been destroyed before attachment completes.
 */
extern tao_shared_object_t* tao_attach_shared_object(tao_error_t** errs,
                                                     int ident, int type);

/**
 * Detach a shared object.
 *
 * Detach a shared object referenced by the caller from the address space of
 * the caller and decrement the reference count of the object.
 *
 * @param errs   Address of a variable to track errors.
 * @param obj    Pointer to a shared object attached to the address space of
 *               the caller.
 *
 * @return `0` on success; `-1` on error.
 */
extern int tao_detach_shared_object(tao_error_t** errs,
                                    tao_shared_object_t* obj);

/**
 * Get the size of a shared object.
 *
 * @param obj    Pointer to a shared object attached to the address space of
 *               the caller.
 *
 * @return The number of bytes of the shared memory segment backing the storage
 * of the shared object.
 */
extern size_t tao_get_shared_object_size(const tao_shared_object_t* obj);

/**
 * Get the type identifier of a shared object.
 *
 * @param obj    Pointer to a shared object attached to the address space of
 *               the caller.
 *
 * @return The type identifier of the shared object.
 */
extern int tao_get_shared_object_type(const tao_shared_object_t* obj);

/**
 * Get the unique identifier of a shared object.
 *
 * @param obj    Pointer to a shared object attached to the address space of
 *               the caller.
 *
 * @return The unique identifier of the shared object.
 */
extern int tao_get_shared_object_ident(const tao_shared_object_t* obj);

/**
 * Lock a shared object.
 *
 * This function locks a shared object that is attached to the address space of
 * the caller.  If the object is locked by another process, the call will block
 * until the object is unlocked by calling tao_unlock_shared_object().
 *
 * @warning The same thread/process must not attempt to lock the same object
 * more than once and should unlock it as soon as possible.
 *
 * @param errs   Address of a variable to track errors.
 * @param obj    Pointer to a shared object attached to the address space of
 *               the caller.
 *
 * @return `0` on success; `-1` on error.
 */
extern int tao_lock_shared_object(tao_error_t** errs, tao_shared_object_t* obj);

/**
 * Attempt to lock a shared object.
 *
 * This function attempts to lock a shared object that is attached to the
 * address space of the caller.  If the object is not currently locked, it
 * becomes locked by the caller and `1` is returned by the call; otherwise, `0`
 * is returned.  This function never blocks.  If `1` is returned, the caller is
 * responsible for eventually unlocking the object by calling
 * tao_unlock_shared_object().
 *
 * @warning The same thread/process must not attempt to lock the same object
 * more than once and should unlock it as soon as possible.
 *
 * @param errs   Address of a variable to track errors.
 * @param obj    Pointer to a shared object attached to the address space of
 *               the caller.
 *
 * @return `1` if the object has been locked by the caller; `0` if the object is
 * already locked; `-1` on error.
 */
extern int tao_try_lock_shared_object(tao_error_t** errs,
                                      tao_shared_object_t* obj);

/**
 * Unlock a shared object.
 *
 * This function unlocks a shared object that has been locked by the caller.
 *
 * @param errs   Address of a variable to track errors.
 * @param obj    Pointer to a shared object attached to the address space of
 *               the caller.
 *
 * @return `0` on success; `-1` on error.
 */
extern int tao_unlock_shared_object(tao_error_t** errs,
                                    tao_shared_object_t* obj);

/** @} */

/*---------------------------------------------------------------------------*/

/**
 * @addtogroup SharedArrays
 *
 * Multi-dimensional arrays whose contents is shared between processes.
 *
 * The elements of a multi-dimensional shared array are all of the same type
 * and are stored contiguously in [colum-major
 * order](https://en.wikipedia.org/wiki/Row-_and_column-major_order).  That is,
 * with their index along the first dimension varying the fastest and their
 * index along the last dimension varying the slowest.
 *
 * Once created, the element type and dimensions of a shared array will remain
 * unchanged.  The values of the elements on a shared array can be modified by
 * the processes providing the shared array is attached to their address space.
 *
 * Like any othe shared objects, each shared array has a unique numerical
 * identifier which is used to retrieve the array and attach it to the address
 * space of the caller.
 *
 * @{
 */

/** Opaque structure to a shared array. */
typedef struct tao_shared_array tao_shared_array_t;

/**
 * Create a new shared array.
 *
 * This function creates a new multi-dimensional array whose contents can be
 * shared between processes.  The returned array is attached to the address
 * space of the caller which is responsible for detaching the shared array when
 * no longer needed by calling tao_detach_shared_array().
 *
 * @param errs   Address of a variable to track errors.
 * @param eltype Identifier of the type of the elements of the array.
 * @param ndims  Number of dimensions of the array.
 * @param dims   Lengths of the dimensions of the array.
 * @param perms  Permissions granted to the group and to the others.
 *
 * @return The address of a new shared array; `NULL` in case of errors.
 *
 * @see tao_create_shared_object.
 */
extern tao_shared_array_t* tao_create_shared_array(tao_error_t** errs,
                                                   tao_eltype_t eltype,
                                                   int ndims, const long dims[],
                                                   unsigned int perms);

/**
 * Create a new mono-dimensional shared array.
 *
 * This function creates a new mono-dimensional array whose contents can be
 * shared between processes.  The returned array is attached to the address
 * space of the caller which is responsible for detaching the shared array when
 * no longer needed by calling tao_detach_shared_array().
 *
 * @param errs   Address of a variable to track errors.
 * @param eltype Identifier of the type of the elements of the array.
 * @param dim    Length of the array.
 * @param perms  Permissions granted to the group and to the others.
 *
 * @return The address of a new shared array; `NULL` in case of errors.
 *
 * @see tao_create_shared_object, tao_create_shared_array.
 */
extern tao_shared_array_t* tao_create_1d_shared_array(tao_error_t** errs,
                                                      tao_eltype_t eltype,
                                                      long dim,
                                                      unsigned int perms);

/**
 * Create a new two-dimensional shared array.
 *
 * This function creates a new two-dimensional array whose contents can be
 * shared between processes.  The returned array is attached to the address
 * space of the caller which is responsible for detaching the shared array when
 * no longer needed by calling tao_detach_shared_array().
 *
 * @param errs   Address of a variable to track errors.
 * @param eltype Identifier of the type of the elements of the array.
 * @param dim1   Length of the first dimension.
 * @param dim2   Length of the second dimension.
 * @param perms  Permissions granted to the group and to the others.
 *
 * @return The address of a new shared array; `NULL` in case of errors.
 *
 * @see tao_create_shared_object, tao_create_shared_array.
 */
extern tao_shared_array_t* tao_create_2d_shared_array(tao_error_t** errs,
                                                      tao_eltype_t eltype,
                                                      long dim1, long dim2,
                                                      unsigned int perms);

/**
 * Create a new three-dimensional shared array.
 *
 * This function creates a new three-dimensional array whose contents can be
 * shared between processes.  The returned array is attached to the address
 * space of the caller which is responsible for detaching the shared array when
 * no longer needed by calling tao_detach_shared_array().
 *
 * @param errs   Address of a variable to track errors.
 * @param eltype Identifier of the type of the elements of the array.
 * @param dim1   Length of the first dimension.
 * @param dim2   Length of the second dimension.
 * @param dim3   Length of the third dimension.
 * @param perms  Permissions granted to the group and to the others.
 *
 * @return The address of a new shared array; `NULL` in case of errors.
 *
 * @see tao_create_shared_object, tao_create_shared_array.
 */
extern tao_shared_array_t* tao_create_3d_shared_array(tao_error_t** errs,
                                                      tao_eltype_t eltype,
                                                      long dim1, long dim2,
                                                      long dim3,
                                                      unsigned int perms);

/**
 * Attach an existing shared array to the address space of the caller.
 *
 * This function attaches an existing shared array to the address space of the
 * caller.  The caller will have a reference on the returned array (whose
 * reference count is therefore incremented by one).  When the array is no
 * longer used by the caller, the caller must call tao_detach_shared_array() to
 * detach the array from its address space, decrement its reference count by
 * one and eventually free the shared memory associated with the array.
 *
 * @param errs   Address of a variable to track errors.
 * @param ident  Unique identifier of the shared object.
 *
 * @return The address of the shared array in the address space of the caller;
 * `NULL` on failure.
 */
extern tao_shared_array_t* tao_attach_shared_array(tao_error_t** errs,
                                                   int ident);

/**
 * Detach a shared array.
 *
 * Detach a shared array referenced by the caller from the address space of the
 * caller and decrement the reference count of the array.
 *
 * @param errs   Address of a variable to track errors.
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller.
 *
 * @return `0` on success; `-1` on error.
 */
extern int tao_detach_shared_array(tao_error_t** errs,
                                   tao_shared_array_t* arr);

/**
 * Get the unique identifier of a shared array.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller.
 *
 * @return The unique identifier of the shared object which stores the array.
 *
 * @see tao_get_shared_object_ident.
 */
extern int tao_get_shared_array_ident(const tao_shared_array_t* arr);

/**
 * Get the type of elements of a shared array.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller.
 *
 * @return One of the `tao_eltype_t` values.
 */
extern tao_eltype_t tao_get_shared_array_eltype(const tao_shared_array_t* arr);

/**
 * Get the number of elements of a shared array.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller.
 *
 * @return The number of elements in the array.
 */
extern long tao_get_shared_array_length(const tao_shared_array_t* arr);

/**
 * Get the number of dimensions of a shared array.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller.
 *
 * @return The number of dimensions of the array.
 */
extern int tao_get_shared_array_ndims(const tao_shared_array_t* arr);

/**
 * Get the length of a dimension of a shared array.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller.
 * @param d      Index of dimension of interest (`1` is the first dimension).
 *
 * @return The number of elements along the given dimension.
 */
extern long tao_get_shared_array_size(const tao_shared_array_t* arr, int d);

/**
 * Get the address of the first element of a shared array.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller.
 *
 * @return The address of the first element of the array.
 */
extern void* tao_get_shared_array_data(const tao_shared_array_t* arr);

/**
 * Get number of readers for a shared array.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller and locked by the caller.
 *
 * @warning Since the number of readers is variable, the caller must have
 * locked the shared array.  For efficiency reasons, this function does not
 * perform error checking.
 *
 * @return The number of readers registered on the shared array.
 *
 * @see tao_lock_shared_array.
 */
extern int tao_get_shared_array_nreaders(const tao_shared_array_t* arr);

/**
 * Adjust number of readers for a shared array.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller and locked by the caller.
 * @param adj    Increment to add to the number of readers.
 *
 * @warning Since the number of readers is variable, the caller must have
 * locked the shared array.  For efficiency reasons, this function does not
 * perform error checking.
 *
 * @return The number of readers registered on the shared array after the
 * adjustment.
 *
 * @see tao_lock_shared_array.
 */
extern int tao_adjust_shared_array_nreaders(tao_shared_array_t* arr, int adj);

/**
 * Get number of writers for a shared array.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller and locked by the caller.
 *
 * @warning Since the number of writers is variable, the caller must have
 * locked the shared array.  For efficiency reasons, this function does not
 * perform error checking.
 *
 * @return The number of writers registered on the shared array.
 *
 * @see tao_lock_shared_array.
 */
extern int tao_get_shared_array_nwriters(const tao_shared_array_t* arr);

/**
 * Adjust number of writers for a shared array.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller and locked by the caller.
 * @param adj    Increment to add to the number of writers.
 *
 * @warning Since the number of writers is variable, the caller must have
 * locked the shared array.  For efficiency reasons, this function does not
 * perform error checking.
 *
 * @return The number of writers registered on the shared array after the
 * adjustment.
 *
 * @see tao_lock_shared_array.
 */
extern int tao_adjust_shared_array_nwriters(tao_shared_array_t* arr, int adj);

/**
 * Get the value of a shared array counter.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller and locked by the caller.
 *
 * @warning Since the counter value is variable, the caller must have
 * locked the shared array.  For efficiency reasons, this function does not
 * perform error checking.
 *
 * @return The counter value of the shared array.
 *
 * @see tao_lock_shared_array.
 */
extern int64_t tao_get_shared_array_counter(const tao_shared_array_t* arr);

/**
 * Set the value of a shared array counter.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller and locked by the caller.
 * @param cnt    Counter value to set.
 *
 * @warning Since the counter value is variable, the caller must have
 * locked the shared array.  For efficiency reasons, this function does not
 * perform error checking.
 *
 * @see tao_lock_shared_array.
 */
extern void tao_set_shared_array_counter(tao_shared_array_t* arr, int64_t cnt);

/**
 * Get the time-stamp of a shared array counter.
 *
 * The time-stamp of a shared array is divided in two integer parts, one gives
 * the integer number of seconds, the other gives the rest as an integer number
 * of nanoseconds.  Thus the resolution of the time-stamp is one nanosecond
 * at best.
 *
 * @param arr     Pointer to a shared array attached to the address space of
 *                the caller and locked by the caller.
 * @param ts_sec  Address to store the integer part of the time-stamp in
 *                seconds.
 * @param ts_nsec Address to store the fractional part of the time-stamp in
 *                nanoseconds.
 *
 * @warning Since the time-stamp is variable, the caller must have
 * locked the shared array.  For efficiency reasons, this function does not
 * perform error checking.
 *
 * @see tao_lock_shared_array.
 */
extern void tao_get_shared_array_timestamp(const tao_shared_array_t* arr,
                                           int64_t* ts_sec, int64_t* ts_nsec);

/**
 * Set the time-stamp of a shared array counter.
 *
 * @param arr     Pointer to a shared array attached to the address space of
 *                the caller and locked by the caller.
 * @param ts_sec  The integer part of the time-stamp in seconds.
 * @param ts_nsec The fractional part  of the time-stamp in nanoseconds.
 *
 * @warning Since the time-stamp is variable, the caller must have
 * locked the shared array.  For efficiency reasons, this function does not
 * perform error checking.
 *
 * @see tao_lock_shared_array.
 */
extern void tao_set_shared_array_timestamp(tao_shared_array_t* arr,
                                           int64_t ts_sec, int64_t ts_nsec);

/**
 * Lock a shared array.
 *
 * @param errs   Address of a variable to track errors.
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller.
 *
 * @return `0` on success; `-1` on error.
 *
 * @see tao_lock_shared_object.
 */
extern int tao_lock_shared_array(tao_error_t** errs,
                                 tao_shared_array_t* arr);

/**
 * Attempt to lock a shared array.
 *
 * @param errs   Address of a variable to track errors.
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller.
 *
 * @return `1` on success; `0` on failure; `-1` on error.
 *
 * @see tao_try_lock_shared_object.
 */
extern int tao_try_lock_shared_array(tao_error_t** errs,
                                     tao_shared_array_t* arr);

/**
 * Unlock a shared array.
 *
 * @param errs   Address of a variable to track errors.
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller.
 *
 * @return `0` on success; `-1` on error.
 *
 * @see tao_unlock_shared_object.
 */
extern int tao_unlock_shared_array(tao_error_t** errs,
                                   tao_shared_array_t* arr);

/** @} */

/*---------------------------------------------------------------------------*/
/* (SHARED) CAMERAS */

/**
 * @addtogroup Cameras
 *
 * The management of every new frame is done in 3 steps.
 *
 * - First, a shared array is located or created to store the resulting data,
 *   if possible an old frame data is recycled.
 *
 * - Second, the raw frame data is processed and stored in the selected shared
 *   array.
 *
 * - Third, the shared array is updated (its counter is incremented) and is
 *   marked as being readable.
 *
 * The first step is done by calling tao_fetch_next_frame() and the third step
 * is done by calling tao_publish_next_frame().  These two functions must be
 * called while the shared camera data have been locked by the caller.  During
 * the pre-processing of the acquired image (2nd step) the shared data should
 * be unlocked and re-locked to call tao_publish_next_frame().
 *
 * @{
 */

/**
 * Number of semaphores associated with a camera.
 */
#define TAO_SHARED_CAMERA_SEMAPHORES 5

/**
 * Opaque shared camera data.
 */
typedef struct tao_shared_camera tao_shared_camera_t;

/**
 * Opaque camera structure for the server.
 */
typedef struct tao_camera tao_camera_t;

/**
 * Region of interest (ROI) in an image.
 *
 * A region of interest (ROI for short) is defined with respect to a *parent*.
 * This parent can be another ROI, the sensor, an image, etc.
 */
typedef struct tao_image_roi {
    long xoff;    /**< Horizontal offset with respect to parent */
    long yoff;    /**< Vertical offset with respect to parent */
    long width;   /**< Horizontal size */
    long height;  /**< Vertical size */
} tao_image_roi_t;

/**
 * Create a camera structure for a frame grabber server.
 *
 * The caller is responsible for eventually calling tao_finalize_camera().
 *
 * @param errs     Address of a variable to track errors.
 * @param nframes  Maximum number of shared arrays to memorize (at least 2).
 * @param perms    Client access permissions for the shared data.
 *
 * @return The address of a camera structure; `NULL` in case of errors.
 */
extern tao_camera_t* tao_create_camera(tao_error_t** errs,
                                       int nframes, unsigned int perms);

/**
 * Finalize a camera structure owned by a frame grabber server.
 *
 * @param errs     Address of a variable to track errors.
 * @param cam      Address of the camera structure.
 *
 * @return `0` on success; `1` in case of errors.
 */
extern int tao_finalize_camera(tao_error_t** errs, tao_camera_t* cam);

/**
 * Get the shared data of a camera structure for a frame grabber server.
 *
 * This function yields the shared camera which is associated with a master
 * camera.
 *
 * @param cam    Pointer to a master camera (must not be `NULL`).
 *
 * @return The address of the shared camera, `NULL` on error.
 *
 * @see tao_attach_shared_camera.
 */
extern tao_shared_camera_t* tao_get_shared_camera(tao_camera_t* cam);

/**
 * Get shared array to store next camera image.
 *
 * This function is called by a frame grabber server when a new image is
 * available.  Its purpose is to provide a suitable shared array to store the
 * new pro-processed image.  If possible, an old shared array from the list
 * owned by the server is recycled.
 *
 * @warning The caller is supposed to have locked the shared camera data.  The
 *          returned array is marked as being written but is unlocked (no one
 *          else is supposed to read/write it).
 *
 * @param errs   Address of a variable to track errors.
 * @param cam    Address of the shared camera data.
 *
 * @return The address of a shared array to store the processed image;
 * `NULL` in case of errors.
 */
tao_shared_array_t* tao_fetch_next_frame(tao_error_t** errs, tao_camera_t* cam);

/**
 * Make a new image available to the clients of a frame grabber server.
 *
 * @warning The caller is supposed to have locked the shared camera data and
 *          @b arr should not be locked by the caller and should have been
 *          obtained by tao_fetch_next_frame().
 *
 * @param errs   Address of a variable to track errors.
 * @param cam    Address of the camera data.
 * @param arr    Address of the shared frame data.
 *
 * @return `0` on success; `1` in case of errors.
 */
int tao_publish_next_frame(tao_error_t** errs, tao_camera_t* cam,
                           tao_shared_array_t* arr);

/**
 * Attach an existing shared camera to the address space of the caller.
 *
 * This function attaches an existing shared camera to the address space of the
 * caller.  The caller will have a reference on the returned camera (whose
 * reference count is therefore incremented by one).  When the camera is no
 * longer used by the caller, the caller must call tao_detach_shared_camera()
 * to detach the camera from its address space, decrement its reference count
 * by one and eventually free the shared memory associated with the camera.
 *
 * @warning The same process must not attach a shared camera more than once.
 *
 * @param errs   Address of a variable to track errors.
 * @param ident  Unique identifier of the shared object.
 *
 * @return The address of the shared camera in the address space of the caller;
 * `NULL` on failure.
 *
 * @see tao_detach_shared_camera, tao_get_shared_camera_ident.
 */
extern tao_shared_camera_t* tao_attach_shared_camera(tao_error_t** errs,
                                                     int ident);

/**
 * Detach a shared camera.
 *
 * Detach a shared camera referenced by the caller from the address space of
 * the caller and decrement the reference count of the camera.
 *
 * @warning Detaching a shared camera does not detach shared arrays backing the
 * storage of the images acquired by this camera.  They have to be explicitly
 * detached.
 *
 * @param errs   Address of a variable to track errors.
 * @param cam    Pointer to a shared camera attached to the address space of
 *               the caller.
 *
 * @return `0` on success; `-1` on error.
 *
 * @see tao_attach_shared_camera.
 */
extern int tao_detach_shared_camera(tao_error_t** errs,
                                    tao_shared_camera_t* cam);

/**
 * Get the identifier of shared camera data.
 *
 * @param cam    Pointer to a shared camera attached to the address space of
 *               the caller and locked by the caller.
 *
 * @return The identifier of the shared camera data.  This value can be used
 *         by another process to attach to its address space the shared camera.
 *         `-1` is returned if @a cam is `NULL`.
 *
 * @see tao_attach_shared_camera.
 */
extern int tao_get_shared_camera_ident(const tao_shared_camera_t* cam);

/**
 * Lock a shared camera.
 *
 * @param errs   Address of a variable to track errors.
 * @param cam    Pointer to a shared camera attached to the address space of
 *               the caller.
 *
 * @return `0` on success; `-1` on error.
 *
 * @see tao_lock_shared_object.
 */
extern int tao_lock_shared_camera(tao_error_t** errs,
                                  tao_shared_camera_t* cam);

/**
 * Attempt to lock a shared camera.
 *
 * @param errs   Address of a variable to track errors.
 * @param cam    Pointer to a shared camera attached to the address space of
 *               the caller.
 *
 * @return `1` on success; `0` on failure; `-1` on error.
 *
 * @see tao_try_lock_shared_object.
 */
extern int tao_try_lock_shared_camera(tao_error_t** errs,
                                      tao_shared_camera_t* cam);

/**
 * Unlock a shared camera.
 *
 * @param errs   Address of a variable to track errors.
 * @param cam    Pointer to a shared camera attached to the address space of
 *               the caller.
 *
 * @return `0` on success; `-1` on error.
 *
 * @see tao_unlock_shared_object.
 */
extern int tao_unlock_shared_camera(tao_error_t** errs,
                                    tao_shared_camera_t* cam);

/**
 * Attach the last acquired image to the address space of the caller.
 *
 * This function attaches the last image acquired by a frame grabber to the
 * address space of the caller.  The caller will have a reference on the
 * returned image (whose reference count is therefore incremented by one).
 * When the image is no longer used by the caller, the caller must call
 * tao_detach_shared_array() to detach the image from its address space,
 * decrement its reference count by one and eventually free the shared memory
 * associated with the image.
 *
 * @warning Since the last image may change (because acquisition is running),
 * the caller is assumed to have locked the shared camera.  For efficiency
 * reasons, this function does not perform error checking.
 *
 * @param errs   Address of a variable to track errors.
 * @param cam    Address of a shared camera attached to the address space of
 *               the caller.
 *
 * The following example shows how to retrieve the last image data providing
 * the image counter has increased since previous image:
 *
 * ```.c
 * tao_shared_camera_t* cam = ...;
 * uint64_t previous_counter = ...;
 * tao_shared_array_t* arr = NULL;
 * tao_lock_shared_camera(NULL, cam);
 * {
 *     uint64_t last_counter = tao_get_last_image_counter(cam);
 *     if (last_counter > previous_counter) {
 *         arr = tao_attach_last_image(NULL, cam);
 *         previous_counter = last_counter;
 *     }
 * }
 * tao_unlock_shared_camera(NULL, cam);
 * ```
 *
 * Note the use of braces to emphasizes the block of statements protected by
 * the lock.  Also note that `NULL` is passed as the address of the variable to
 * track errors so any error will be considered as fatal in this example.
 *
 * The same example without tao_attach_last_image():
 *
 * ```.c
 * tao_shared_camera_t* cam = ...;
 * uint64_t previous_counter = ...;
 * tao_shared_array_t* arr = NULL;
 * tao_lock_shared_camera(NULL, cam);
 * {
 *     uint64_t last_counter = tao_get_last_image_counter(cam);
 *     if (last_counter > previous_counter) {
 *         int ident = tao_get_last_image_ident(cam);
 *         if (ident >= 0) {
 *             arr = tao_attach_shared_array(NULL, ident);
 *             previous_counter = last_counter;
 *         }
 *     }
 * }
 * tao_unlock_shared_camera(NULL, cam);
 * ```
 *
 * @return The address of the shared camera in the address space of the caller;
 * `NULL` on failure or if there is no valid last image.
 */
extern tao_shared_array_t* tao_attach_last_image(tao_error_t** errs,
                                                 tao_shared_camera_t* cam);

/**
 * Get the current state of the camera.
 *
 * @param cam   Address of shared camera in address space of caller.
 *
 * @return `-1` if @a cam is `NULL`; otherwise, `0` if device not yet open, `1`
 *         if device open but no acquisition is running, `2` if acquisition is
 *         running.
 */
extern int tao_get_shared_camera_state(const tao_shared_camera_t* cam);

/**
 * Get the pixel type for the captured images after pre-processing.
 *
 * @param cam   Address of shared camera in address space of caller.
 *
 * @return The pixel type for the captured images after pre-processing, `-1`
 *         if @a cam is `NULL`.
 */
extern int tao_get_shared_camera_pixeltype(const tao_shared_camera_t* cam);

/**
 * Get the depth of the raw images.
 *
 * @param cam   Address of shared camera in address space of caller.
 *
 * @return The number of bits per pixel in the raw captured images,
 *         `0` if @a cam is `NULL`.
 */
extern int tao_get_shared_camera_depth(const tao_shared_camera_t* cam);

/**
 * Get the width of the detector.
 *
 * This function yields the number of pixels per line of the detector which is
 * the maximum width for captured iamges.
 *
 * @param cam   Address of shared camera in address space of caller.
 *
 * @return The number of pixels per line of the detector, `0` if @a cam is
 *         `NULL`.
 */
extern long tao_get_shared_camera_sensorwidth(const tao_shared_camera_t* cam);

/**
 * Get the height of the detector.
 *
 * This function yields the number of lines of pixels of the detector which is
 * the maximum height for captured iamges.
 *
 * @param cam   Address of shared camera in address space of caller.
 *
 * @return The number of lines of pixels of the detector, `0` if @a cam is
 *         `NULL`.
 */
extern long tao_get_shared_camera_sensorheight(const tao_shared_camera_t* cam);

/**
 * Get the horizontal binning factor.
 *
 * @param cam   Address of shared camera in address space of caller.
 *
 * @return The horizontal binning factor in physical pixels, `0` if
 *         @a cam is `NULL`.
 */
extern long tao_get_shared_camera_xbin(const tao_shared_camera_t* cam);

/**
 * Get the vertical binning factor.
 *
 * @param cam   Address of shared camera in address space of caller.
 *
 * @return The vertical binning factor in physical pixels, `0` if
 *         @a cam is `NULL`.
 */
extern long tao_get_shared_camera_ybin(const tao_shared_camera_t* cam);

/**
 * Get the horizontal offset of captured images.
 *
 * @param cam   Address of shared camera in address space of caller.
 *
 * @return The horizontal offset in physical pixels of the region of interest
 *         set for the captured images, `0` if @a cam is `NULL`.
 */
extern long tao_get_shared_camera_xoff(const tao_shared_camera_t* cam);

/**
 * Get the vertical offset of captured images.
 *
 * @param cam   Address of shared camera in address space of caller.
 *
 * @return The vertical offset in physical pixels of the region of interest
 *         set for the captured images, `0` if @a cam is `NULL`.
 */
extern long tao_get_shared_camera_yoff(const tao_shared_camera_t* cam);

/**
 * Get the width of the captured images.
 *
 * This function yields the number of macro-pixels per line of the captured
 * images.  If no sub-sampling nor re-binning of physical pixels is used a
 * macro-pixel corresponds to a physical pixel.
 *
 * @param cam   Address of shared camera in address space of caller.
 *
 * @return The number of macro-pixels per line of the captured images, `0` if
 *         @a cam is `NULL`.
 */
extern long tao_get_shared_camera_width(const tao_shared_camera_t* cam);

/**
 * Get the height of the captured images.
 *
 * This function yields the number of lines of macro-pixels in the captured
 * images.  If no sub-sampling nor re-binning of physical pixels is used a
 * macro-pixel corresponds to a physical pixel.
 *
 * @param cam   Address of shared camera in address space of caller.
 *
 * @return The number of lines of macro-pixels in the captured images, `0` if
 *         @a cam is `NULL`.
 */
extern long tao_get_shared_camera_height(const tao_shared_camera_t* cam);

/**
 * Get the frame rate.
 *
 * @param cam   Address of shared camera in address space of caller.
 *
 * @return The number of frame per seconds, `0` if @a cam is `NULL`.
 */
extern double tao_get_shared_camera_framerate(const tao_shared_camera_t* cam);

/**
 * Get the duration of the exposure.
 *
 * @param cam   Address of shared camera in address space of caller.
 *
 * @return The exposure time in seconds for the captured images, `0` if @a cam
 *         is `NULL`.
 */
extern double tao_get_shared_camera_exposuretime(const tao_shared_camera_t* cam);

/**
 * Wait for next acquired image.
 *
 * This function waits for a new image to be available from a given camera.  If
 * a new image is already available, the function returns immediately;
 * otherwise the call blocks until a new image becomes available or a signal
 * handler interrupts the call.
 *
 * @warning Each process waiting for a new image should use a different
 * semaphore index.
 *
 * @param errs   Address of a variable to track errors.
 * @param cam    Pointer to a shared camera attached to the address space of
 *               the caller.
 * @param sem    Index (starting at 1) of semaphore to use.
 *
 * @return `0` on success; `-1` on error.
 */
extern int tao_wait_image(tao_error_t** errs, tao_shared_camera_t* cam,
                          int sem);

/**
 * Attempt to wait for next acquired image.
 *
 * This function checks whether a new image is immediately available from a
 * given camera.  Compared to tao_wait_image(), this function never blocks and
 * return immediately.  After this function returns `1`, this function can only
 * return `1` again after the next new image (on the same camera and index @a
 * sem).
 *
 * @warning Each process waiting for a new image should use a different
 * semaphore index.
 *
 * @param errs   Address of a variable to track errors.
 * @param cam    Pointer to a shared camera attached to the address space of
 *               the caller.
 * @param sem    Index (starting at 1) of semaphore to use.
 *
 * @return `1` if a new image is available; `0` if no image is currently
 * available; `-1` in case of error.
 */
extern int tao_try_wait_image(tao_error_t** errs, tao_shared_camera_t* cam,
                              int sem);

/**
 * Attempt to wait for next acquired image.
 *
 * This function behaves like tao_wait_image() but blocks no longer than some
 * given duration.  After this function returns `1`, this function can only
 * return `1` again after the next new image (on the same camera and index
 * @a sem).
 *
 * @warning Each process waiting for a new image should use a different
 * semaphore index.
 *
 * @param errs   Address of a variable to track errors.
 * @param cam    Pointer to a shared camera attached to the address space of
 *               the caller.
 * @param sem    Index (starting at 1) of semaphore to use.
 * @param secs   Maximum time to wait (in seconds).  If this amount of time is
 *               too small (less than a nanosecond), the effect is the same as
 *               calling tao_try_wait_image().  If the amount of time is too
 *               large (more than one year), the effect is the same as calling
 *               tao_wait_image().
 *
 * @return `1` if a new image is available before the specified number of
 * seconds; `0` if timeout occured before a new image becomes available; `-1`
 * in case of error.
 *
 * @see tao_get_absolute_timeout.
 */
extern int tao_timed_wait_image(tao_error_t** errs, tao_shared_camera_t* cam,
                                int sem, double secs);

/**
 * Get the counter value of the last acquired image.
 *
 * @param cam    Pointer to a shared camera attached to the address space of
 *               the caller and locked by the caller.
 *
 * @warning Since the last image may change (because acquisition is running),
 * the caller must have locked the shared camera.  For efficiency reasons, this
 * function does not perform error checking.
 *
 * @return The value of the counter of the last acquired image, `0` if none
 *         or if @a cam is `NULL`.
 *
 * @see tao_lock_shared_camera.
 */
extern uint64_t tao_get_last_image_counter(const tao_shared_camera_t* cam);

/**
 * Get the identifier of the last acquired image.
 *
 * @param cam    Pointer to a shared camera attached to the address space of
 *               the caller and locked by the caller.
 *
 * @warning Since the last image may change (because acquisition is running),
 * the caller must have locked the shared camera.  For efficiency reasons, this
 * function does not perform error checking.
 *
 * @return The identifier of the last acquired image, `-1` if none or
 *         if @a cam is `NULL`.
 *
 * @see tao_lock_shared_camera.
 */
extern int tao_get_last_image_ident(const tao_shared_camera_t* cam);

/** @} */

/** @} */

/*---------------------------------------------------------------------------*/

/**
 *  @addtogroup RealTimeProcessing
 *
 * @{
 */

/**
 * Apply image pre-processing.
 *
 * The following formula is applied for each pixel index `i` to compute the
 * pixel value and its weight:
 *
 * ```
 * d[i] = (r[i] - b[i])*a[i];
 * w[i] = u[i]/(max(d[i],0) + v[i]);
 * ```
 * @param d     Ouput image data.
 * @param w     Ouput image weights.
 * @param r     Input raw image data.
 * @param n     Number of pixels.
 * @param a     Gain correction term.
 * @param b     Bias correction term.
 * @param u     Numerator for weights.
 * @param v     Denominator for weights.
 *
 * If @a w is non `NULL` but @a u or @ v is `NULL`, all weights are set to `1`.
 * If @a w is `NULL`, no weights are computed.
 */
extern void tao_preprocess_image_u8_to_f32(float* d, float* w,
                                           const uint8_t* r, int n,
                                           const float* a, const float* b,
                                           const float* u, const float* v);

/**
 * Apply image pre-processing.
 *
 * @see tao_preprocess_image_u8_to_f32.
 */
extern void tao_preprocess_image_u8_to_f64(double* d, double* w,
                                           const uint8_t* r, int n,
                                           const double* a, const double* b,
                                           const double* u, const double* v);

/**
 * Apply image pre-processing.
 *
 * @see tao_preprocess_image_u8_to_f32.
 */
extern void tao_preprocess_image_u16_to_f32(float* d, float* w,
                                            const uint16_t* r, int n,
                                            const float* a, const float* b,
                                            const float* u, const float* v);

/**
 * Apply image pre-processing.
 *
 * @see tao_preprocess_image_u8_to_f32.
 */
extern void tao_preprocess_image_u16_to_f64(double* d, double* w,
                                            const uint16_t* r, int n,
                                            const double* a, const double* b,
                                            const double* u, const double* v);
/** @} */

/**
 * @addtogroup Utilities
 *
 * @{
 *
 *     @addtogroup CopyConvert
 *
 *     @{
 */

/**
 * Copy/convert regions of multi-dimensional arrays.
 *
 * This function copies (and possibly converts) the elements of a rectangular
 * region between two multi-dimensional arrays.
 *
 * @param errs     Address of a variable to track errors.
 * @param dstdata  Address of first element in destination array.
 * @param dsttype  Type of elements in destination array.
 * @param dstdims  Dimensions of destination array.
 * @param dstoffs  Offsets of destination region (can be `NULL` if there are
 *                 no offsets).
 * @param srcdata  Address of first element in source array.
 * @param srctype  Type of elements in source array.
 * @param srcdims  Dimensions of source array.
 * @param srcoffs  Offsets of source region (can be `NULL` if there are
 *                 no offsets).
 * @param lens     Dimensions of region to copy.
 * @param ndims    Number of dimensions (length of @b dstdims, @b dstoffs,
 *                 @b srcdims, @b srcoffs and @b lens).
 *
 * @return `0` on success, `-1` on error.
 *
 * @see tao_copy_checked_args().
 */
extern int tao_copy(tao_error_t** errs,
                    void* dstdata, tao_eltype_t dsttype,
                    const long dstdims[], const long dstoffs[],
                    const void* srcdata, tao_eltype_t srctype,
                    const long srcdims[], const long srcoffs[],
                    const long lens[], int ndims);

/**
 * Copy/convert regions of multi-dimensional arrays.
 *
 * This function is the same as tao_copy() except that no checking of the
 * arguments is performed.
 *
 * @param dstdata  Address of first element in destination array.
 * @param dsttype  Type of elements in destination array.
 * @param dstdims  Dimensions of destination array.
 * @param dstoffs  Offsets of destination region (can be `NULL` if there are
 *                 no offsets).
 * @param srcdata  Address of first element in source array.
 * @param srctype  Type of elements in source array.
 * @param srcdims  Dimensions of source array.
 * @param srcoffs  Offsets of source region (can be `NULL` if there are
 *                 no offsets).
 * @param lens     Dimensions of region to copy.
 * @param ndims    Number of dimensions (length of @b dstdims, @b dstoffs,
 *                 @b srcdims, @b srcoffs and @b lens).
 *
 * @see tao_copy().
 */
extern void tao_copy_checked_args(void* dstdata, tao_eltype_t dsttype,
                                  const long dstdims[], const long dstoffs[],
                                  const void* srcdata, tao_eltype_t srctype,
                                  const long srcdims[], const long srcoffs[],
                                  const long lens[], int ndims);

/**
 * Set a region into an array.
 */
extern int tao_copy_to_array(tao_error_t** errs,
                             tao_array_t* dst, const long dstoffs[],
                             const void* srcdata, tao_eltype_t srctype,
                             const long srcdims[], const long srcoffs[],
                             const long lens[], int ndims);

/**
 * Set a region into a shared array.
 */
extern int tao_copy_to_shared_array(tao_error_t** errs,
                                    tao_shared_array_t* dst,
                                    const long dstoffs[],
                                    const void* srcdata, tao_eltype_t srctype,
                                    const long srcdims[], const long srcoffs[],
                                    const long lens[], int ndims);

/**
 * Extract a region from an array.
 */
extern int tao_copy_from_array(tao_error_t** errs,
                               void* dstdata, tao_eltype_t dsttype,
                               const long dstdims[], const long dstoffs[],
                               tao_array_t* src, const long srcoffs[],
                               const long lens[], int ndims);

/**
 * Extract a region from a shaded array.
 */
extern int tao_copy_from_shared_array(tao_error_t** errs,
                                      void* dstdata, tao_eltype_t dsttype,
                                      const long dstdims[],
                                      const long dstoffs[],
                                      tao_shared_array_t* src,
                                      const long srcoffs[],
                                      const long lens[], int ndims);

/**
 * Copy a region of an array into another array.
 */
extern int tao_copy_array_to_array(tao_error_t** errs,
                                   tao_array_t* dst, const long dstoffs[],
                                   tao_array_t* src, const long srcoffs[],
                                   const long lens[], int ndims);

/**
 * Copy a region of an array into a shared array.
 */
extern int tao_copy_array_to_shared_array(tao_error_t** errs,
                                          tao_shared_array_t* dst,
                                          const long dstoffs[],
                                          tao_array_t* src,
                                          const long srcoffs[],
                                          const long lens[], int ndims);

/**
 * Copy a region of a shared array into an array.
 */
extern int tao_copy_shared_array_to_array(tao_error_t** errs,
                                          tao_array_t* dst,
                                          const long dstoffs[],
                                          tao_shared_array_t* src,
                                          const long srcoffs[],
                                          const long lens[], int ndims);

/**
 * Copy a region of a shared array into another shared array.
 */
extern int tao_copy_shared_array_to_shared_array(tao_error_t** errs,
                                                 tao_shared_array_t* dst,
                                                 const long dstoffs[],
                                                 tao_shared_array_t* src,
                                                 const long srcoffs[],
                                                 const long lens[], int ndims);

/**
 *     @}
 *
 * @}
 */

_TAO_END_DECLS

#endif /* _TAO_H_ */
