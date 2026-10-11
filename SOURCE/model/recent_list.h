#ifndef GMENU_MODEL_RECENT_LIST_H
#define GMENU_MODEL_RECENT_LIST_H

#include "limits.h"

/* The folder names of the recently played games, the newest first. */
struct RecentList {
  enum { Size = 10 };

  char folders[Size][FolderSize];

  /* To the front (when it is not there, the last one drops off). */
  void add( const char *folder );
};

#endif
