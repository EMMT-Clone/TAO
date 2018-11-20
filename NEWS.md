* Shared images may have associated weights.

* Implement the policy for sharing images allowing no readers if there is a
  writer and at most one writer.

* C type `long` is used for array dimensions, number of elements and indices.

* Error messages are correctly printed in Julia interface.

* Anonymous semaphores are used to signal new images.
