* For some reasons, calling the dynamic TAO library from Yorick fails:

  > ERROR (*main*) plug_in: unable to find dynamic library file
  > WARNING detailed line number information unavailable

  To solve this, Yorick plugin to TAO contains its own version of the
  compiled objects files of the library.
