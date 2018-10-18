/*
 * logmsg.c --
 *
 * Logging messages.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018, Éric Thiébaut.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>

#include "config.h"
#include "tao-private.h"

/*--------------------------------------------------------------------------*/
/* ERROR HANDLER */

/* Mutex to protect message handler settings. */
static pthread_mutex_t message_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Current level of verbosity (protected by above mutex). */
static tao_message_type_t message_level = TAO_DEBUG;

/* Current message output file (protected by above mutex). */
static FILE* message_output = NULL;

static const char*
message_prefix(tao_message_type_t type)
{
  switch (type) {
  case TAO_DEBUG:
    return "[DEBUG]  ";
  case TAO_INFO:
    return "[INFO]   ";
  case TAO_WARN:
    return "[WARN]   ";
  case TAO_ERROR:
    return "[ERROR]  ";
  case TAO_ASSERT:
    return "[ASSERT] ";
  default:
    return "[?????]  ";
  }
}

void
tao_set_message_level(tao_message_type_t level)
{
  if (level < TAO_DEBUG) {
    level = TAO_DEBUG;
  }
  if (level > TAO_QUIET) {
    level = TAO_QUIET;
  }
  if (pthread_mutex_lock(&message_mutex) == 0) {
    message_level = level;
    (void)pthread_mutex_unlock(&message_mutex);
  }
}

tao_message_type_t
tao_get_message_level()
{
  tao_message_type_t level = TAO_DEBUG;
  if (pthread_mutex_lock(&message_mutex) == 0) {
    level = message_level;
    (void)pthread_mutex_unlock(&message_mutex);
  }
  return level;
}

void
tao_inform(tao_message_type_t type, const char* format, ...)
{
  if (pthread_mutex_lock(&message_mutex) == 0) {
    if (type >= message_level) {
      va_list ap;
      FILE* output = (message_output == NULL ? stderr : message_output);
      (void)fputs(message_prefix(type), output);
      va_start(ap, format);
      (void)vfprintf(output, format, ap);
      va_end(ap);
    }
    (void)pthread_mutex_unlock(&message_mutex);
  }
}
