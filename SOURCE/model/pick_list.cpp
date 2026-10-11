#include <string.h>

#include "pick_list.h"
#include "../lib/core/str.h"

void PickList::clear() {
  count = 0;
  poolEnd_ = pool_;
}

void PickList::addRef( const char *value ) {
  if ( count < Capacity ) items[count++] = value;
}

void PickList::addCopy( const char *value ) {
  if ( count >= Capacity || poolEnd_ > pool_ + PoolSize - MaxGenre - 1 ) return;
  items[count++] = poolEnd_;
  strcpy( poolEnd_, value );
  poolEnd_ += strlen( value ) + 1;
}

int PickList::find( const char *value, int first ) const {
  for ( int i = first; i < count; i++ ) {
    if ( cmpi( value, items[i] ) == 0 ) return i;
  }
  return -1;
}
