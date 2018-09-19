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
 * Copyright (C) 2018, Éric Thiébaut.
 */

#ifndef _TAO_H_
#define _TAO_H_ 1

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

/**
 * \defgroup Messages      Messages and Logging
 * \defgroup Errors        Error Reporting
 * \defgroup SharedObjects Shared Objects
 * \defgroup SharedArrays  Shared Multi-Dimensional Arrays.
 * \defgroup Utilities     Low-Level and Utility Functions
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
 * \addtogroup Messages
 * @{
 */

typedef enum tao_message_type {
  /* Values must be in ascending order. */
  TAO_DEBUG  = 0,  /**< Debug message */
  TAO_INFO   = 1,  /**< Information message */
  TAO_WARN   = 2,  /**< Warning message */
  TAO_ERROR  = 3,  /**< Runtime error */
  TAO_ASSERT = 4,  /**< Assertion error or bug */
  TAO_QUIET  = 5   /**< Suppress all messages */
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
 *                instance, `TAO_QUIET` to suppress printing of messages or
 *                `TAO_DEBUG` to print everything.
 */
extern void tao_set_message_level(tao_message_type_t level);

/** @} */

/*---------------------------------------------------------------------------*/

/**
 * \addtogroup Errors
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
 *   (Some TAO functions may return a result which may take a specific value,
 *   say `NULL` or `-1`, when an error occured.)  It is the caller
 *   responsibility to ensure that errors are eventually taken into account.
 *   There are basically two possibilities: errors can be reported with
 *   tao_report_errors() or errors can be simply ignored with the
 *   tao_discard_errors().  Calling any of these two functions does free the
 *   ressources associated with the memorized errors.
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
 * Error codes.
 *
 * Errors in TAO are identified by an integer.  Strictly positive values
 * indicate a system error using the same identifiers as `errno`.  Strictly
 * negative values indicate a TAO error.  Zero (that is, `TAO_SUCCESS`)
 * indicate a successful operation.
 */
typedef enum tao_error_code {
    TAO_SUCCESS          =   0, /**< Operation was successful */
    TAO_BAD_MAGIC        =  -1, /**< Invalid magic number */
    TAO_BAD_SERIAL       =  -2, /**< Invalid serial number */
    TAO_BAD_SIZE         =  -3, /**< Invalid size */
    TAO_BAD_TYPE         =  -4, /**< Invalid type */
    TAO_BAD_RANK         =  -5, /**< Invalid number of dimensions */
    TAO_DESTROYED        =  -6, /**< Ressource has been destroyed */
    TAO_SYSTEM_ERROR     =  -7, /**< Unknown system error */
    TAO_CANT_TRACK_ERROR =  -8  /**< Insufficient memory for tracking error */
} tao_error_code_t;

/**
 * Register an error due to a function call.
 *
 * This function is called to register the information related to the occurence
 * of an error.  This information consiste in the name of the function @p func
 * where the error occured and the numerical identifier @p code of the error.
 * If @p errs is non-`NULL`, it is assumed to be that of the variable provided
 * by the caller to track errors and the error information is added there;
 * othwerise (that is, if @p errs is `NULL`), the error is immediately reported
 * and the process is aborted.
 *
 * @param errs   Address of a variable to track errors.
 * @param func   Name of the function where the error occured.
 * @param code   Error identifier.
 */
extern void
tao_push_error(tao_error_t** errs, const char* func, int code);

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
extern void
tao_push_system_error(tao_error_t** errs, const char* func);

/**
 * Pop last tracked error.
 *
 * This function pops information about the last error remaining in the list of
 * errors tracked by the variable at address @p errs.  The errors are popped in
 * reverse temporal order.  That is, the last occuring error is retrieved
 * first.  Ressources associated with the popped error are freed.
 *
 * The following example demonstrates how to use tao_pop_error() to report
 * all errors that occured:
 *
 * ```.c
 * void report_errors(tao_error_t** errs)
 * {
 *     int code;
 *     const char* func;
 *     while (tao_pop_error(errs, &func, &code)) {
 *         fprintf(stderr, "error %d in %s\n", code, func);
 *     }
 * }
 * ```
 *
 * @param errs      Address of a variable to track errors.
 * @param funcptr   Address of a variable to store the name of the function
 *                  where the last remaining error occured.  Can be `NULL` to
 *                  not retrieve this information.
 * @param codeptr   Address of a variable to store the code of the last
 *                  remaining error.  Can be `NULL` to not retrieve this
 *                  information.
 *
 * @return A boolean value (that is, `0` or `1`) indicating whether there was
 * some error information to retrieve.
 */
extern int
tao_pop_error(tao_error_t** errs, const char** funcptr, int* codeptr);

/**
 * Report all tracked errors.
 *
 * This function prints the errors in the given list in reverse order of
 * occurence and delete the contents of the error list.
 *
 * @param errs   Address of a variable to track errors.
 */
extern void
tao_report_errors(tao_error_t** errs);

/**
 * Clear all tracked errors.
 *
 * This function delete the contents of the error list.
 *
 * @param errs   Address of a variable to track errors.
 */
extern void
tao_discard_errors(tao_error_t** errs);

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
extern const char*
tao_get_error_reason(int code);

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
extern const char*
tao_get_error_name(int code);

/** @} */

/*---------------------------------------------------------------------------*/

/**
 * \addtogroup Utilities
 *
 * Utilities functions to wrap system calls so as to report
 * errors in the TAO way.
 *
 * @{
 */

/**
 * Allocate dynamic memory.
 *
 * This function behaves as malloc() except that error information may be
 * tracked in @p errs.  The caller is responsible to call free() or tao_free()
 * to free the allocated memory when no longer needed.
 *
 * @param errs   Address of a variable to track errors.
 * @param size   Number of bytes to allocate.
 *
 * @return The address of allocated dynamic memory; `NULL` in case of errors.
 *
 * @see tao_free, tao_calloc.
 */
extern void*
tao_malloc(tao_error_t** errs, size_t size);

/**
 * Allocate dynamic memory.
 *
 * This function behaves as calloc() except that error information may be
 * tracked in @p errs.  The caller is responsible to call free() or tao_free()
 * to free the allocated memory when no longer needed.
 *
 * @param errs   Address of a variable to track errors.
 * @param nelem  Number of elements to allocate.
 * @param elsize Number of bytes per element.
 *
 * @return The address of allocated dynamic memory; `NULL` in case of errors.
 *
 * @see tao_free, tao_malloc.
 */
extern void*
tao_calloc(tao_error_t** errs, size_t nelem, size_t elsize);

/**
 * Free dynamic memory.
 *
 * This function behaves as free() except that it accepts a `NULL` pointer.
 *
 * @param ptr    Address of dynamic memory (can be `NULL`).
 *
 * @see tao_malloc, tao_calloc.
 */
extern void
tao_free(void* ptr);

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
 *
 * @return `0` if successful; `-1` in case of error.
 */
extern int tao_initialize_mutex(tao_error_t** errs, pthread_mutex_t* mutex,
                                int shared);

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
 * @param errs   Address of a variable to track errors.
 * @param mutex  Pointer to the mutex to destroy.
 *
 * @return `0` if successful; `-1` in case of error.
 */
extern int tao_destroy_mutex(tao_error_t** errs, pthread_mutex_t* mutex);

/** @} */

/*---------------------------------------------------------------------------*/

/**
 * \addtogroup SharedObjects
 *
 * Generic objects whose contents is shared between processes.
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
    TAO_FRAME_GRABBER = (TAO_SHARED_MAGIC | 2), /**< Frame grabber shared
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
 * The remaining bytes after the basic object information are set to zero.
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
extern tao_shared_object_t*
tao_create_shared_object(tao_error_t** errs, tao_object_type_t type,
                         size_t size, unsigned int perms);

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
 * @warning The same process must not attach a shared object more than once.
 *
 * @param errs   Address of a variable to track errors.
 * @param ident  Unique identifier of the shared object.
 * @param type   Expected type identifier of the shared object.  Can be
 *               `TAO_SHARED_ANY` to accept any shared object type.
 *
 * @return The address of the shared object in the address space of the caller;
 * `NULL` on failure.
 */
extern tao_shared_object_t*
tao_attach_shared_object(tao_error_t** errs, int ident, int type);

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
extern int
tao_detach_shared_object(tao_error_t** errs, tao_shared_object_t* obj);

/**
 * Get the size of a shared object.
 *
 * @param obj    Pointer to a shared object attached to the address space of
 *               the caller.
 *
 * @return The number of bytes of the shared memory segment backing the storage
 * of the shared object.
 */
extern size_t
tao_get_shared_object_size(const tao_shared_object_t* obj);

/**
 * Get the type identifier of a shared object.
 *
 * @param obj    Pointer to a shared object attached to the address space of
 *               the caller.
 *
 * @return The type identifier of the shared object.
 */
extern int
tao_get_shared_object_type(const tao_shared_object_t* obj);

/**
 * Get the unique identifier of a shared object.
 *
 * @param obj    Pointer to a shared object attached to the address space of
 *               the caller.
 *
 * @return The unique identifier of the shared object.
 */
extern int
tao_get_shared_object_ident(const tao_shared_object_t* obj);

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
extern int
tao_lock_shared_object(tao_error_t** errs, tao_shared_object_t* obj);

/**
 * Attempt to lock a shared object.
 *
 * This function attempts to lock a shared object that is attached to the
 * address space of the caller.  If the object is not currently locked, it
 * becomes locked by the caller and `1` is returned by the call; otherwise, `0`
 * is returned.  This function never blocks.  If `1` is returned, the caller is
 * responsible of eventually unlocking the object by calling
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
extern int
tao_try_lock_shared_object(tao_error_t** errs, tao_shared_object_t* obj);

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
extern int
tao_unlock_shared_object(tao_error_t** errs, tao_shared_object_t* obj);

/** @} */

/*---------------------------------------------------------------------------*/

/**
 * \addtogroup SharedArrays
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

/** Maximun number of dimensions of (shared) arrays. */
#define TAO_MAX_NDIMS 5

/** The identifier of the type of the elements in a shared arrays. */
typedef enum tao_element_type {
    TAO_INT8    =   1, /**< Signed 8-bit integer */
    TAO_UINT8   =   2, /**< Unsigned 8-bit integer */
    TAO_INT16   =   3, /**< Signed 16-bit integer */
    TAO_UINT16  =   4, /**< Unsigned 16-bit integer */
    TAO_INT32   =   5, /**< Signed 32-bit integer */
    TAO_UINT32  =   6, /**< Unsigned 32-bit integer */
    TAO_INT64   =   7, /**< Signed 64-bit integer */
    TAO_UINT64  =   8, /**< Unsigned 64-bit integer */
    TAO_FLOAT32 =   9, /**< 32-bit floating-point */
    TAO_FLOAT64 =  10  /**< 64-bit floating-point */
} tao_element_type_t;

/**
 * Get the size of an array element given its type.
 *
 * @param eltype Identifier of the type of the elements of an array.
 *
 * @return A strictly positive number of bytes if @p eltype is valid;
 * `0` is @p eltype is not valid.
 *
 * @see tao_element_type_t.
 */
extern size_t
tao_get_element_size(int eltype);

/**
 * Create a new shared array.
 *
 * This function creates a new multi-dimensional array whose contents can be
 * shared between processes.  The returned array is attached to the address
 * space of the caller which is responsible to detach the shared array when no
 * longer needed by calling tao_detach_shared_array().
 *
 * @param errs   Address of a variable to track errors.
 * @param eltype Identifier of the type of the elements of the array.
 * @param ndims  Number of dimensions of the array.
 * @param size   Lengths of the dimensions of the array.
 * @param perms  Permissions granted to the group and to the others.
 *
 * @return The address of a new shared array; `NULL` in case of errors.
 *
 * @see tao_create_shared_object.
 */
extern tao_shared_array_t*
tao_create_shared_array(tao_error_t** errs, tao_element_type_t eltype,
                        int ndims, const size_t size[], unsigned int perms);

/**
 * Create a new 1D shared array.
 *
 * This function creates a new 1D array whose contents can be shared between
 * processes.  The returned array is attached to the address space of the
 * caller which is responsible to detach the shared array when no longer needed
 * by calling tao_detach_shared_array().
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
extern tao_shared_array_t*
tao_create_1d_shared_array(tao_error_t** errs, tao_element_type_t eltype,
                           size_t dim, unsigned int perms);

/**
 * Create a new 2D shared array.
 *
 * This function creates a new 3D array whose contents can be shared between
 * processes.  The returned array is attached to the address space of the
 * caller which is responsible to detach the shared array when no longer needed
 * by calling tao_detach_shared_array().
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
extern tao_shared_array_t*
tao_create_2d_shared_array(tao_error_t** errs, tao_element_type_t eltype,
                           size_t dim1, size_t dim2, unsigned int perms);

/**
 * Create a new 3D shared array.
 *
 * This function creates a new 3D array whose contents can be shared between
 * processes.  The returned array is attached to the address space of the
 * caller which is responsible to detach the shared array when no longer needed
 * by calling tao_detach_shared_array().
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
extern tao_shared_array_t*
tao_create_3d_shared_array(tao_error_t** errs, tao_element_type_t eltype,
                           size_t dim1, size_t dim2, size_t dim3,
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
 * @warning The same process must not attach a shared array more than once.
 *
 * @param errs   Address of a variable to track errors.
 * @param ident  Unique identifier of the shared object.
 *
 * @return The address of the shared array in the address space of the caller;
 * `NULL` on failure.
 */
extern tao_shared_array_t*
tao_attach_shared_array(tao_error_t** errs, int ident);

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
extern int
tao_detach_shared_array(tao_error_t** errs, tao_shared_array_t* arr);

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
extern int
tao_get_array_ident(const tao_shared_array_t* arr);

/**
 * Get the type of elements of a shared array.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller.
 *
 * @return One of the `tao_element_type_t` values.
 */
extern int
tao_get_array_eltype(const tao_shared_array_t* arr);

/**
 * Get the number of elements of a shared array.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller.
 *
 * @return The number of elements in the array.
 */
extern size_t
tao_get_array_length(const tao_shared_array_t* arr);

/**
 * Get the number of dimensions of a shared array.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller.
 *
 * @return The number of dimensions of the array.
 */
extern int
tao_get_array_ndims(const tao_shared_array_t* arr);

/**
 * Get the length of a dimension of a shared array.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller.
 * @param d      Index of dimension of interest (`1` is the first dimension).
 *
 * @return The number of elements along the given dimension.
 */
extern size_t
tao_get_array_size(const tao_shared_array_t* arr, int d);

/**
 * Get the address of the first element of a shared array.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller.
 *
 * @return The address of the first element of the array.
 */
extern void*
tao_get_array_data(const tao_shared_array_t* arr);

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
extern int
tao_get_array_nreaders(const tao_shared_array_t* arr);

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
extern int
tao_adjust_array_nreaders(tao_shared_array_t* arr, int adj);

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
extern int
tao_get_array_nwriters(const tao_shared_array_t* arr);

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
extern int
tao_adjust_array_nwriters(tao_shared_array_t* arr, int adj);

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
extern int64_t
tao_get_array_counter(const tao_shared_array_t* arr);

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
extern void
tao_set_array_counter(tao_shared_array_t* arr, int64_t cnt);

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
extern void
tao_get_array_timestamp(const tao_shared_array_t* arr,
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
extern void
tao_set_array_timestamp(tao_shared_array_t* arr,
                        int64_t ts_sec, int64_t ts_nsec);

/**
 * Lock a shared array.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller.
 *
 * @return `0` on success; `-1` on error.
 *
 * @see tao_lock_shared_object.
 */
extern int
tao_lock_shared_array(tao_error_t** errs, tao_shared_array_t* arr);

/**
 * Attempt to lock a shared array.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller.
 *
 * @return `1` on success; `0` on failure; `-1` on error.
 *
 * @see tao_try_lock_shared_object.
 */
extern int
tao_try_lock_shared_array(tao_error_t** errs, tao_shared_array_t* arr);

/**
 * Unlock a shared array.
 *
 * @param arr    Pointer to a shared array attached to the address space of
 *               the caller.
 *
 * @return `0` on success; `-1` on error.
 *
 * @see tao_unlock_shared_object.
 */
extern int
tao_unlock_shared_array(tao_error_t** errs, tao_shared_array_t* arr);

/** @} */

_TAO_END_DECLS

#endif /* _TAO_H_ */
