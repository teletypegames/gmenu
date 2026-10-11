#include <string.h>

#include "executable_list.h"

void ExecutableList::clear() {
  count = 0;
}

void ExecutableList::add( const char *name ) {
  char x[FolderSize];
  memset( x, 0, sizeof( x ) );
  strncpy( x, name, FolderSize - 1 );
  if ( count >= Capacity ) return;
  int i;
  for ( i = 0; i < count && memcmp( x, names[i], FolderSize ) >= 0; i++ );
  memmove( names[i + 1], names[i], ( count - i ) * FolderSize );
  memcpy( names[i], x, FolderSize );
  count++;
}
