/*
   sort.cpp - the sort orders of the game list (F9).
*/

#include "gmenu.h"

char SortMode = 0;
const char *const SortNames[NSORT] = { "title", "year", "genre", "most played" };

/* By title, then by folder. */
int cmpTitle( int a, int b ) {
  int r = cmpi( Games[a].title, Games[b].title );
  return r ? r : cmpi( Games[a].folder, Games[b].folder );
}

/* By a field (empty last), then by title. */
static int cmpField( int a, int b, int field ) {
  const char *x = getField( a, field );
  const char *y = getField( b, field );
  if ( !*x || !*y ) {
    if ( *x ) return -1;
    if ( *y ) return 1;
    return cmpTitle( a, b );
  }
  int r = cmpi( x, y );
  return r ? r : cmpTitle( a, b );
}

static int cmpYear( int a, int b ) {
  return cmpField( a, b, F_YEAR );
}

static int cmpGenre( int a, int b ) {
  return cmpField( a, b, F_GENRE );
}

/* GPtrs in the order of SortMode */
void sortGames( void ) {
  static int ( *const cmps[NSORT] )( int, int ) = { cmpTitle, cmpYear, cmpGenre, cmpPlays };
  shellSort( GPtrs, GCount, cmps[SortMode] );
}
