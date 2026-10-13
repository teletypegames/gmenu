#include <stdlib.h>

#include "visible_games.h"

VisibleGames::VisibleGames() : games( NULL ), count( 0 ), sel( 0 ), top( 0 ), rows( 1 ), capacity_( 0 ) {}

VisibleGames::~VisibleGames() {
  free( games );
}

bool VisibleGames::reserve( int n ) {
  if ( n <= capacity_ ) return true;
  int *more = (int *)realloc( games, n * sizeof( int ) );
  if ( more == NULL ) return false;
  games = more;
  capacity_ = n;
  return true;
}

int VisibleGames::current() const {
  return count ? games[sel] : -1;
}

bool VisibleGames::select( int game ) {
  for ( int i = 0; i < count; i++ ) {
    if ( games[i] == game ) {
      sel = i;
      return true;
    }
  }
  return false;
}

void VisibleGames::center() {
  top = sel - rows / 2;
  if ( top < 0 ) top = 0;
}
