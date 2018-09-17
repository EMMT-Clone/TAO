## Error Reporting

Error reporting in TAO C library is simple: all functions of the library which
may report an error have a first parameter which is the address of a status
code.  (Note: it must be the first one because there are functions with a
variable number of arguments.)  Initially, the status must be set to zero
(`TAO_SUCCESS`).  On entry, such a function checks the status value; if it is
non-zero, the function returns with the status value unchanged; otherwise, the
function proceeeds but will set the status value to a non-zero error code on
error.

At any point, if status value is zero, it means that all calls were successful
so far.  If the status value is strictly positive, it corresponds to a system
error code (as given by `errno`).  If the status value is strictly negative, it
corresponds to a specific error in the library.  Function
`tao_get_error_text` may be used to convert the error code into a human
readable message.

```c
tao_error_t* errs = NULL;
tao_fun1(&errs, arg1, arg2, ...);
tao_fun2(&errs, ...);
tao_fun3(&errs, ...);
if (TAO_ANY_ERRORS(&errs)) {
    tao_report_errors(errs);
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
