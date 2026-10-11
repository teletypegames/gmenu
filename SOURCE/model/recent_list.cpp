#include <string.h>

#include "recent_list.h"
#include "../lib/core/str.h"

void RecentList::add( const char *folder ) {
  int i;
  for ( i = 0; i < Size - 1 && cmpi( folders[i], folder ); i++ );
  for ( ; i > 0; i-- ) memcpy( folders[i], folders[i - 1], FolderSize );
  strcpy( folders[0], folder );
}
