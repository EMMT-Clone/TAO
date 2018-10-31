* Setting connection speed is not fully functional (it works sometimes).

* Implement connection-reset.

* Implement saving/restoring of camera configuration to/from ROM.

* Set trigger mode (even though defaults are OK).

* Close then open camera connection (or send a connection reset) when
  a certain kind of errors happens to fix spurious errors when setting
  some parameters like the pixel format.

* The way the end of acquisition is signaled makes the "stop" command behaves
  as the "abort" one.
