/*
 * sharedobjects.c --
 *
 * Implement objects whose contents can be shared between processes.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018, Éric Thiébaut.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "config.h"
#include "macros.h"
#include "tao-private.h"

/* Shared objects */

/*

Shared objects are stored in shared memory so as to be accessible to any
process (providing they have granted permissions).  Because shared objects have
a number of attributes (*e.g.*, their reference count) that must be updated,
the shared memory backing the object storage must have read-write access
granted.  Thus, at least the user who creates the shared object have read-write
access to the object.  Access for the group and for the others can be freely
chosen by the creator.

- The object manager resources are stored in shared memory accessible to all
  processes and monitored by a mutex (stored in the same shared memory
  segment).

- Integer fields in structures have a defined size and signedness to facilitate
  interfacing with other languages than C.

- When the number of references `nrefs` becomes zero, the object is assumed to
  be no longer in use and can be safely destroyed.  In particular, once the
  number of references become zero, it can not be incremented.  For debugging
  purposes, the number of references is a signed integer (even though the
  previous rules implies that the number of references can never be negative).

- The field `ident` is a numerical identifier of the object.  All shared object
  have a unique identifier (at a given time).  The identifier may have the
  value `-1`, to indicate an invalid value or an undefined object.  In
  practice, the identifier can be the BSD System V `shmid` or the index of a
  path name for named (POSIX) shared memory.

  If POSIX shared memory is used, the unique numerical identifier of the object
  is created by incrementing a counter when the object is created.  The counter
  is stored in shared memory in the global object manager ressources.  The path
  name is composed as:

  ```.c
  sprintf(path, "tao-%s-%d.shm", typeName, ident);
  ```

  where `typeName` is the name of the object type (unique associated with the
  value of the `type` filed of a `tao_shared_object_t` structure).

  If BSD System V shared memory is used, the unique identifer is automatically
  created by using the special key `IPC_PRIVATE`:

  ```.c
  ident = shmid(IPC_PRIVATE, size, flags);
  ```

- The type of the object is uniquely associated with the type name of the
  object.  To allow for fast checking, the 24 most significant bits of the type
  match `TAO_SHARED_MAGIC`.  The specific object type are encoded in the
  remaining 8 least significant bits.  So that there are 256 possible different
  object types.  For instance:
*/
#define PERMS_MASK (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)

tao_shared_object_t*
tao_create_shared_object(tao_error_t** errs, tao_object_type_t type,
                         size_t size, mode_t perms)
{
    int failures = 0;

    /* Check arguments. */
    if ((type & TAO_SHARED_MASK) != TAO_SHARED_MAGIC) {
        tao_push_error(errs, __func__, TAO_BAD_MAGIC);
        return NULL;
    }
    if (size < TAO_SHARED_MIN_SIZE || size > TAO_SHARED_MAX_SIZE) {
        tao_push_error(errs, __func__, TAO_BAD_SIZE);
        return NULL;
    }

    /* Create a new segment of shared memory and get its identifier. */
    int flags = (perms & PERMS_MASK) | (S_IRUSR|S_IWUSR|IPC_CREAT|IPC_EXCL);
    int ident = shmget(IPC_PRIVATE, size, flags);
    if (ident == -1) {
        tao_push_system_error(errs, "shmget");
        return NULL;
    }

    /* Attach the shared memory segment to the address space of the caller. */
    void *ptr = shmat(ident, NULL, 0);
    if (ptr == (void*)-1) {
        tao_push_system_error(errs, "shmat");
        ++failures;
    }

    /* Manage to destroy the shared memory segment associated with the object
       on last detach. */
    if (shmctl(ident, IPC_RMID, NULL) == -1) {
        tao_push_system_error(errs, "shmctl");
        ++failures;
    }

    /* If any errors occured so far, return NULL. */
    if (failures > 0) {
        return NULL;
    }

    /* Instanciate the object. */
    tao_shared_object_t* obj = (tao_shared_object_t*)ptr;
    (void)memset(obj, 0, size);
    if (tao_initialize_mutex(errs, &obj->mutex, 1) == -1) {
        if (shmdt(ptr) == -1) {
            tao_push_system_error(errs, "shmdt");
        }
        return NULL;
    }
    obj->nrefs = 1;
    obj->ident = ident;
    obj->size = size;
    obj->type = type;
    return obj;
}

tao_shared_object_t*
tao_attach_shared_object(tao_error_t** errs, int ident, int type)
{
    /* Check arguments. */
    if (ident < 0) {
        tao_push_error(errs, __func__, TAO_BAD_SERIAL);
        return NULL;
    }

    /* Attach shared memory segment to the address space of the caller.
       An error here means that either the identifier is wrong or that the
       object has been destroyed. */
    void* ptr = shmat(ident, NULL, 0);
    if (ptr == (void*)-1) {
        tao_push_system_error(errs, "shmat");
        return NULL;
    }
    tao_shared_object_t* obj = (tao_shared_object_t*)ptr;

    /* Retrieve the actual size of the object and check the validity of the
       object before locking it. */
    struct shmid_ds info;
    if (shmctl(ident, IPC_STAT, &info) == -1) {
        tao_push_system_error(errs, "shmctl");
        goto err;
    }
    size_t size = info.shm_segsz;
    if (size < TAO_SHARED_MIN_SIZE || size != obj->size) {
        tao_push_error(errs, __func__, TAO_BAD_SIZE);
        goto err;
    }
    if ((obj->type & TAO_SHARED_MASK) != TAO_SHARED_MAGIC) {
        tao_push_error(errs, __func__, TAO_BAD_MAGIC);
        goto err;
    }
    if (type != TAO_SHARED_ANY && obj->type != type) {
        tao_push_error(errs, __func__, TAO_BAD_TYPE);
        goto err;
    }

    /* We must refuse to re-use an object that has been marked for being no
       longer used and thus is about to be destroyed.  This must be done before
       locking the object because, if the object is being destroyed, destroying
       its associated mutex would fail if it is locked.  The check is repeated
       just after locking the object.  See comments in
       tao_detach_shared_object for explanations. */
    if (obj->nrefs <= 0) {
        goto destroyed;
    }
    if (pthread_mutex_lock(&obj->mutex) != 0) {
        /* Assume that locking failed because the object has been destroyed in
           the mean time. */
        goto destroyed;
    }
    if (obj->nrefs <= 0) {
        /* Unlock object immediately and report that object has been destroyed.
           Unlocking should not cause any errors here. */
        pthread_mutex_unlock(&obj->mutex);
        goto destroyed;
    }
    ++obj->nrefs;
    if (tao_unlock_shared_object(errs, obj) == -1) {
        /* This is not supposed to happen. */
        goto err;
    }
    return obj;

    /* Any error after attaching the memory segment branch here. */
 err:
    if (shmdt(ptr) == -1) {
        tao_push_system_error(errs, "shmdt");
    }
    return NULL;

    /* Object has been destroyed. */
 destroyed:
    tao_push_error(errs, __func__, TAO_DESTROYED);
    goto err;
}

int
tao_detach_shared_object(tao_error_t** errs, tao_shared_object_t* obj)
{
    int status = 0;

    /* Check arguments. */
    if (obj == NULL) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }

    /* Decrease the reference count of the object and detach the shared memory
       segment from the address space of the calling process. */
    if (tao_lock_shared_object(errs, obj) == -1) {
        status = -1;
        goto detach;
    }
    int nrefs = --obj->nrefs;
    if (tao_unlock_shared_object(errs, obj) == -1) {
        status = -1;
        goto detach;
    }

    /* If the object is no longer in use, destroy the associated ressources
       before detaching the object from the address space of the calling
       process. */
    if (nrefs == 0) {
        /* Destroy the shared mutex.  One issue arises from the fact that the
           mutex must be unocked while being destroyed (see
           `pthread_mutex_destroy(3)`).  The mutex must be unlocked just before
           being destroyed to minimize the risk that another process locks the
           mutex in the mean time.  To reduce the risk, when a shared object is
           attached, we check its reference count before locking it and report
           that object is about to be destroyed if the reference count is less
           or equal zero.  Then we lock the mutex and re-check its reference
           count and if it is less or equal zero, we immediately unlock the
           mutex and report that object is about to be destroyed.  But the risk
           still exists that `pthread_mutex_destroy(3)` is called when the
           mutex is locked and returns `EBUSY`.  In that case, we wait a bit to
           let the other process unlock the mutex and re-call
           `pthread_mutex_destroy(3)`.  This is repeated untill success. */
        long nsec = 2000; /* wait 2µs if first attempt fails */
        while (1) {
            int code = pthread_mutex_destroy(&obj->mutex);
            if (code == 0) {
                /* Success. */
                break;
            } else if (code == EBUSY) {
                struct timespec ts;
                if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
                    tao_push_system_error(errs, "clock_gettime");
                    status = -1;
                    break;
                }
                ts.tv_nsec += nsec;
                if (nanosleep(&ts, NULL) != 0) {
                    tao_push_system_error(errs, "nanosleep");
                    status = -1;
                    break;
                }
                nsec += nsec; /* double next waiting duration */
            } else {
                /* Failure. */
                tao_push_error(errs, "pthread_mutex_destroy", code);
                status = -1;
                break;
            }
        }

        /* Destroy other resources (semaphores, etc.). */
        if (obj->type == TAO_SHARED_CAMERA) {
            /* Note that destroying a semaphore that other processes or threads
               are currently blocked on (in sem_wait(3)) produces undefined
               behavior.  This should not be the case because when the object
               is destroyed no process should be using any of its resources. */
            tao_shared_camera_t* cam = (tao_shared_camera_t*)obj;
            for (int i = 0; i < TAO_SHARED_CAMERA_SEMAPHORES; ++i) {
                (void)sem_destroy(&cam->sem[i]);
            }
        }
    }

    /* Detach the shared memory segment from the address space of the calling
       process. */
 detach:
    if (shmdt((void*)obj) == -1) {
        tao_push_system_error(errs, "shmdt");
        status = -1;
    }

    /* Return whether operation was successful. */
    return status;
}

size_t
tao_get_shared_object_size(const tao_shared_object_t* obj)
{
    return (likely(obj != NULL) ? obj->size : 0);
}

int
tao_get_shared_object_type(const tao_shared_object_t* obj)
{
    return (likely(obj != NULL) ? obj->type : -1);
}

int
tao_get_shared_object_ident(const tao_shared_object_t* obj)
{
    return (likely(obj != NULL) ? obj->ident : -1);
}

int
tao_lock_shared_object(tao_error_t** errs, tao_shared_object_t* obj)
{
    if (unlikely(obj == NULL)) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    return tao_lock_mutex(errs, &obj->mutex);
}

int
tao_try_lock_shared_object(tao_error_t** errs, tao_shared_object_t* obj)
{
    if (unlikely(obj == NULL)) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    return tao_try_lock_mutex(errs, &obj->mutex);
}

int
tao_unlock_shared_object(tao_error_t** errs, tao_shared_object_t* obj)
{
    if (unlikely(obj == NULL)) {
        tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
        return -1;
    }
    return tao_unlock_mutex(errs, &obj->mutex);
}
