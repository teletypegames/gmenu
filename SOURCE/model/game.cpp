#include <string.h>

#include "game.h"
#include "../lib/core/str.h"

const char *Game::field( int f ) const {
  const char *s = record;
  if ( s == NULL ) return "";
  while ( f-- ) s += strlen( s ) + 1;
  return s;
}

bool Game::isFavorite() const {
  return ( flags & GameFavorite ) != 0;
}

static char *twoDigits( char *d, unsigned n ) {
  *d++ = (char)( '0' + n / 10 );
  *d++ = (char)( '0' + n % 10 );
  return d;
}

void Game::playedText( char *out ) const {
  if ( plays == 0 ) {
    strcpy( out, "never" );
    return;
  }
  char *d = numStr( out, plays );
  strcpy( d, "x, last " );
  d = numStr( d + strlen( d ), ( last >> 9 ) + 1980 );
  *d++ = '-';
  d = twoDigits( d, ( last >> 5 ) & 15 );
  *d++ = '-';
  d = twoDigits( d, last & 31 );
  *d = 0;
}

int compareTitles( const Game &a, const Game &b ) {
  int r = cmpi( a.title, b.title );
  return r ? r : cmpi( a.folder, b.folder );
}
