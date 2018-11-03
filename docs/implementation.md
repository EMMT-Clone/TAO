## Error Reporting

Errors in TAO are identified by a numerical code (see `tao_error_code_t`) and
by the name of the function where the error occured.  When an error occurs in
most TAO function, this information may be transferred to the caller via the
first parameter of the function.  (Note: it must be the first argument because
there are functions with a variable number of arguments.)  To collect error
information, the caller has to provide the address of a pointer to an opaque
`tao_error_t` structure.  There are two possibilities:

- If the address of the variable to track the errors is `NULL`, then any
  occuring error is assumed to be fatal: it is immediately reported and the
  process is aborted.

- If the address of the variable to track the errors is not `NULL`, then
  information about errors that have occured are stored (as a chained list) in
  the variable.  Initially, the variable to track error information must be set
  to `TAO_NO_ERRORS` to indicate that there are no errors yet.  To check
  whether an error has occured, the caller just have to check whether the
  variable is not equal to `TAO_NO_ERRORS` after calling a TAO function.  (Some
  TAO functions may also return a specific value, say `NULL` or `-1`, when an
  error occured.)  The macro `TAO_ANY_ERRORS()` can also be used to check
  whether any error occured.  It is the caller responsibility to ensure that
  errors are eventually taken into account.  There are basically two
  possibilities: errors can be reported with `tao_report_errors() `or errors
  can be simply ignored with the `tao_discard_errors()`.  Calling any of these
  two functions does free the resources associated with the memorized errors.

Note that, if the (small) amount of memory needed to store the error
information cannot be allocated, a fatal error is assumed: the current error
and all pending errors are immediately reported and the process is aborted.

Usually, functions should stop upon the first ocurring error.  This is however
not always possible (for instance when an object is destroyed several resources
may have to be destroyed).  When there are several errors occuring in the same
function call, the error mangement mechanism implemented in TAO offers the
possibility to report all errors that occured.

Example:

```c
    tao_error_t* errs = TAO_NO_ERRORS;
    if (tao_fun1(&errs, arg1, arg2, ...) != 0) {
        goto done;
    }
    ...; // do something
    if (tao_fun2(&errs, ...) != 0 ||
        tao_fun3(&errs, ...) != 0) {
        goto done;
    }
    ...; // do something else
done:
  if (errs != TAO_NO_ERRORS) {
      tao_report_errors(&errs);
      exit(1);
  }
```

## Shared Objects

Shared objects are stored in shared memory so as to be accessible to any
process (providing they have granted permissions).  Because shared objects have
a number of attributes (*e.g.*, their reference count) that must be updated,
the shared memory backing the object storage must have read-write access
granted.  Thus, at least the user who creates the shared object have read-write
access to the object.  Access for the group and for the others can be freely
chosen by the creator.

* The object manager resources are stored in shared memory accessible to all
  processes and monitored by a mutex (stored in the same shared memory
  segment).

* Integer fields in structures have a defined size and signedness to facilitate
  interfacing with other languages than C.

* When the number of references `nrefs` becomes zero, the object is assumed to
  be no longer in use and can be safely destroyed.  In particular, once the
  number of references become zero, it can not be incremented.  For debugging
  purposes, the number of references is a signed integer (even though the
  previous rules implies that the number references can never be negative).

* The field `ident` is a numerical identifier of the object.  All shared object
  have a unique identifier (at a given time).  The identifier may have the
  value `-1`, to indicate an invalid value or an undefined object.  In
  practice, the identifier can be the BSD System V `shmid` or the index of a
  path name for named (POSIX) shared memory.

  If POSIX shared memory is used, the unique numerical identifier of the object
  is created by incrementing a counter when the object is created.  The counter
  is stored in shared memory in the global object manager ressources.  The path
  name is composed as:

  ```c
  sprintf(path, "tao-%s-%d.shm", typeName, ident);
  ```

  where `typeName` is the name of the object type (unique associated with the
  value of the `type` filed of a `TaoSharedObject` structure).

  If BSD System V shared memory is used, the unique identifer is automatically
  created by using the special key `IPC_PRIVATE`:

  ```c
  ident = shmid(IPC_PRIVATE, size, flags);
  ```

* The type of the object is uniquely associated with the type name of the
  object.  To allow for fast checking, the 24 most significant bits of the type
  match `TAO_SHARED_MAGIC`.  The specific object type are encoded in the
  remaining 8 least significant bits.  So that there are 256 possible different
  object types.  For instance:

  ```c
  #define TAO_SHARED_MAGIC   0x310efc00
  typedef enum TaoObjectType {
      TAO_SHARED_OBJECT = (TAO_SHARED_MAGIC | 0),
      TAO_SHARED_IMAGE  = (TAO_SHARED_MAGIC | 1),
      TAO_SHARED_ANY    = 0xffffffff
  } TaoObjectType;
  ```

* The member `mutex` is used to monitor the usage of the object.

* All shared objects have a structure which starts with the same members as
  `TaoSharedObject`.  Memory beyond `sizeof(TaoSharedObject)` is used to store
  other members and data (with suitable alignment).

```c
typedef struct TaoSharedObject {
    pthread_mutex_t mutex;  /* mutex associated with the object */
    int32_t         nrefs;  /* number of references on the object */
    uint32_t        size;   /* totla number of bytes allocated for the object */
    int32_t         ident;  /* numerical identifier of the ressource */
    int32_t         type;   /* object type */
} TaoSharedObject;
```


```c
TaoSharedObject* taoCreateSharedObject(TaoObjectType type, size_t size);
TaoSharedObject*
taoCreateSharedObject(TaoObjectType type, size_t size)
{
    void* ptr;
    int ident;
    TaoSharedObject* obj;

    ident = shmid(IPC_PRIVATE, size, S_IUSR|IPC_CREATE|IPC_EXCL);
    ptr = shmat(ident, NULL, ...);
    obj = (TaoSharedObject*)ptr;
    initialize_mutex(&obj->mutex);
    obj->nrefs = 1;
    obj->size = size;
    obj->ident = ident;
    obj->type = type;
    return obj;
}
```

```c
TaoSharedObject* taoAttachSharedObject(TaoObjectType type, int32_t ident);
TaoSharedObject*
taoAttachSharedObject(TaoObjectType type, int32_t ident)
{
    void* ptr;
    size_t size;
    TaoSharedObject* obj;

    ptr = shmat(ident, NULL, 0);
    obj = (TaoSharedObject*)ptr;
    /* retrieve the actual size */
    size = ...;
    if (size < sizeof(TaoSharedObject)) {
        shmdt(ptr);
        return NULL;
    }
    if (obj->size != size) {
        shmdt(ptr);
        return NULL;
    }
    if ((obj->type & 0xFFFFFF00) != TAO_SHARED_MAGIC) {
        shmdt(ptr);
        return NULL;
    }
    if (type != TAO_SHARED_ANY && obj->type != type) {
        shmdt(ptr);
        return NULL;
    }
    if (obj->nrefs <= 0) {
        shmdt(ptr);
        return NULL;
    }
    taoLockSharedObject(obj);
    ++obj->nrefs;
    taoUnlockSharedObject(obj);
    return obj;
}
```


Detaching a shared object is done by:

```c
void taoDetachSharedObject(TaoSharedObject* obj);

void
taoDetachSharedObject(TaoSharedObject* obj)
{
    /* Decrease the reference count of the object and detach the shared memory
       segment from the address space of the calling process. */
    taoLocksharedobject(obj);
    --obj->nrefs;
    taoUnlockSharedObject(obj);
    if (shmdt((void*)obj) == -1) {
    }
}
```

A garbage collector is in charge of effectively destroying no longer referenced
shared objects.
