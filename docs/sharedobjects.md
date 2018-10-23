# Shared Objects

TAO implements a variety of shared objects whose contents is stored in shared
memory so that they can be accessed by different processes.  Among others:

- **shared cameras** are shared objects used to store parameters about the
  images acquired by a camera;

- **shared arrays** are shared objects used to store multi-dimensional arrays
  like images;


## Locks

Any TAO shared object can be locked.  Locks are used to prevent some shared
data to be modified by other processes during some (limited) amount of time.
To avoid blocking other processes, an object should be locked during the
shortest possible duration.

For shared arrays which may be accessed in read only mode by several processes
at the same time, additional rules apply to allow protected access by several
processes while avoiding locking the array and preventing access to others.

The proposed policy is that if a process, called a **reader**, wants to read
some information or some values in a shared array and makes sure that the array
will not change in the mean time, it increases the number of readers of the
array.  The counterpart for a process, called a **writer**, which wants to
change the contents of a shared array is that this can only be done if there
are no registered readers nor other writers.  This implies that (i) there can
be at most one writer at any time, (ii) there must be no writers while there is
at least one reader, and since read-only access cannot be granted if there is a
writer, (iii) there must be no readers while there is a writer.

The pseudo-code for a *reader* is:

```julia
# Try to gain read-only access.
lock(arr)
if arr.nwriters > 0
    # Some process is writing.
    success = false
else
    arr.nreaders += 1
    success = true
end
unlock(arr)
if success
    # Read the array contents.
    ...
    # Drop read-only access.
    lock(arr)
    arr.nreaders -= 1
    unlock(arr)
end
```

The pseudo-code for a *writer* is:

```julia
# Try to gain read-write access.
lock(arr)
if arr.nreaders > 0 || arr.nwriters > 0
    # Some processes are reading or writing.
    success = false
else
    arr.nwriters += 1
    success = true
end
unlock(arr)
if success
    # Write the array contents.
    for i in 1:arr.nelem
        arr.data[i] = ...
    end
    # Drop read-write access.
    lock(arr)
    arr.nwriters -= 1
    unlock(arr)
end
```

Shared arrays have different kinds of contents, some attributes are immutable
and can be read at any time without locking the array.  The attributes of a
shared array are summarized in the following table (the *Member* column should
be considered as pseudo-code):

| Member      | Modifiable | Description                |
|:----------- |:---------- |:-------------------------- |
| `ident`     | no         | Shared object identifier   |
| `type`      | no         | Shared object type         |
| `eltype`    | no         | Type of array elements     |
| `ndims`     | no         | Number of dimensions       |
| `size[d]`   | no         | Length of dimension `d`    |
| `nelem`     | no         | Number of elements         |
| `nwriters`  | yes        | Number of writers          |
| `nreaders`  | yes        | Number of readers          |
| `counter`   | yes        | Image counter              |
| `timestamp` | yes        | Image time-stamp           |
| `data[i]`   | yes        | Value of array element `i` |

For shared arrays used to store acquired images, applying the above policy may
result in having readers which prevent updating the shared array contents
immediately.  There are two possible solutions:

1. The protected access policy holds and the writer (the frame grabber
   process), drops usage on the shared array (that is *detach* the shared array)
   if it has any readers and allocate a new one.  Creating a new shared array
   takes a few microseconds (4-5µs). **FIXME:** Exactly how many time does this
   takes?  This potential delay may be avoided by always having a *spare*
   shared array pre-allocated (and re-allocated after the image pre-processing
   stage if the spare array has been consumed).

2. It may be decided that the writer has the priority and is allowed to update
   the array contents regardless whether it has readers or not.  In order for
   readers to be able to realize that the array has been overwritten, the
   writer takes care of updating the array counter (and perhaps its time-stamp)
   before changing the values of the array elements.

Pseudo-code for the writer when a new image has been acquired and will be
stored in shared array `arr`:

```julia
# First indicates that the array is being updated and increments its counter.
lock(arr)
arr.nwriters = 1
arr.counter += 1
unlock(arr)
# Then update array contents.
for i in 1:arr.nelem
    arr.data[i] = ...
end
# Finally clear the writing flag and update global information.
lock(arr)
arr.nwriters = 0
unlock(arr)
lock(cam)
cam.last_frame.ident = arr.ident
unlock(cam)
```

where `cam` is the shared camera instance.  Setting the `nwriters` member is to
let readers known that the array contents is being overwritten before trying to
access its contents.  Pseudo-code for a reader:

```julia
# Attach shared array storing the last image and memorize its counter.
lock(cam)
ident = cam.last_frame.ident
unlock(cam)
arr = attach(ident)
if arr == NULL
    # Array was destroyed.
    success = false
else
    lock(arr)
    if arr != NULL && arr.nwriters == 0
        counter = arr.counter
        success = true
    else
        success = false
    end
    unlock(arr)
end
if success
    # Read the array contents.
    ...
    # Check whether array contents has been overwritten.
    lock(arr)
    if arr.nwriters != 0 || arr.counter != counter
        success = false
    end
    unlock(arr)
end
if success
    # Pursue the processing.
    ...
else
    # Discard any results of the processing.
    ...
end
```

The reader must be prepared to face that `attach` fails because the shared
array has been destroyed before the reader can attach it (hence the test `arr
!= NULL`).  The array (as any other shared object) cannot be destroyed while it
is attached by at least one process.
