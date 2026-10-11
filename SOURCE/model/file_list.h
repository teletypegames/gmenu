/*
   The files of a game's folder, sorted by name.
*/

#ifndef GMENU_MODEL_FILE_LIST_H
#define GMENU_MODEL_FILE_LIST_H

#include "limits.h"

struct FileList {
  enum { Capacity = 600 };

  char names[Capacity][FolderSize];
  const char *sorted[Capacity];
  int count;

  void clear();
  /* false when the list is full */
  bool add( const char *name );
  void sort();
};

#endif
