* Use `struct timespec` for storing the time (instead of `tao_time_t` which was
  completely similar).

* Manage to have '-Wall -Werror -O2' the default compilation flags.

* Include `stdbool.h` and use the macros provided by this header: `bool` for
  the type of a boolean and `true` or `false` values.

* Shared images may have associated weights.

* Implement the policy for sharing images allowing no readers if there is a
  writer and at most one writer.

* C type `long` is used for array dimensions, number of elements and indices.

* Error messages are correctly printed in Julia interface.

* Anonymous semaphores are used to signal new images.
