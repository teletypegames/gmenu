#include "visible_games.h"

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
