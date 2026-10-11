/*
   The values of a picker (the filters, the recently played games).
*/

#ifndef GMENU_MODEL_PICK_LIST_H
#define GMENU_MODEL_PICK_LIST_H

#include "recent_list.h"

struct PickList {
  enum { Capacity = 250, PoolSize = 6000 };

  const char *items[Capacity];
  int count;
  int games[RecentList::Size]; /* the games of the recently played picker */

  void clear();
  /* A value that lives elsewhere. */
  void addRef( const char *value );
  /* A copy of the value (dropped when the list is full). */
  void addCopy( const char *value );
  /* The index of the value (case-insensitive) from first on, or -1. */
  int find( const char *value, int first ) const;

private:
  char pool_[PoolSize];
  char *poolEnd_;
};

#endif
