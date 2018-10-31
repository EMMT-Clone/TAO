* Setting connection speed is not fully functional (it works sometimes).

* Implement connection-reset.

* Implement saving/restoring of camera configuration to/from ROM.

* Set trigger mode (even though defaults are OK).

* The way the end of acquisition is signaled makes the "stop" command behaves
  as the "abort" one.

* Some operations (mostly playing with the configuration) should be forbidden
  while acquisition is running.

* phx_start should apply the user chosen configuration?
