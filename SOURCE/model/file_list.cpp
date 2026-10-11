#include <string.h>

#include "file_list.h"
#include "../lib/core/sort.h"
#include "../lib/core/str.h"

void FileList::clear() {
  count = 0;
}

bool FileList::add( const char *name ) {
  if ( count >= Capacity ) return false;
  memcpy( names[count], name, FolderSize );
  sorted[count] = names[count];
  count++;
  return true;
}

void FileList::sort() {
  shellSort( sorted, count, cmpi );
}
