/*
   The executables of a game's folder, sorted by name (at most 100).
*/

#ifndef GMENU_MODEL_EXECUTABLE_LIST_H
#define GMENU_MODEL_EXECUTABLE_LIST_H

#include "limits.h"

struct ExecutableList {
  enum { Capacity = 100 };

  char names[Capacity][FolderSize];
  int count;

  void clear();
  /* In its place (dropped when the list is full). */
  void add( const char *name );
};

#endif
