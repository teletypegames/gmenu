#include <string.h>

#include "filter_criteria.h"
#include "game.h"
#include "../lib/core/str.h"

bool FilterCriteria::filtersActive() const {
  return genre[0] || year[0] || players[0] || favoritesOnly || noMetadataOnly;
}

void FilterCriteria::clearFilters() {
  genre[0] = year[0] = players[0] = 0;
  favoritesOnly = noMetadataOnly = 0;
}

void FilterCriteria::clearSearch() {
  searchLen = 0;
  search[0] = 0;
}

bool FilterCriteria::addToSearch( char c ) {
  if ( searchLen >= MaxSearch ) return false;
  search[searchLen++] = c;
  search[searchLen] = 0;
  return true;
}

bool FilterCriteria::backspace() {
  if ( searchLen == 0 ) return false;
  search[--searchLen] = 0;
  return true;
}

char *FilterCriteria::filterOf( int field ) {
  if ( field == FieldGenre ) return genre;
  if ( field == FieldYear ) return year;
  return players;
}

static bool parseNum( const char **s, unsigned *n ) {
  const char *p = *s;
  *n = 0;
  while ( *p >= '0' && *p <= '9' ) *n = *n * 10 + ( *p++ - '0' );
  if ( p == *s ) return false;
  *s = p;
  return true;
}

bool parseRange( const char **s, unsigned *a, unsigned *b ) {
  if ( !parseNum( s, a ) ) return false;
  *b = *a;
  *s = skipBlank( *s );
  if ( **s == '+' ) *b = 0xFFFF;
  else if ( **s == '-' ) {
    *s = skipBlank( *s + 1 );
    if ( !parseNum( s, b ) ) *b = *a;
  }
  return true;
}

static bool isDecade( const char *filter ) {
  return strlen( filter ) == 5 && ( filter[4] | 0x20 ) == 's';
}

static bool matchField( const Game &game, int field, const char *filter ) {
  if ( !*filter ) return true;
  const char *v = game.field( field );
  if ( field == FieldPlayers ) {
    const char *p = filter;
    unsigned n, a, b;
    if ( parseNum( &p, &n ) ) {
      p = v;
      if ( parseRange( &p, &a, &b ) ) return n >= a && n <= b;
    }
  }
  else if ( field == FieldYear && isDecade( filter ) ) {
    return v[0] == filter[0] && v[1] == filter[1] && v[2] == filter[2] && v[3] >= '0' && v[3] <= '9';
  }
  return cmpi( v, filter ) == 0;
}

static bool contains( const char *text, const char *part ) {
  for ( ; *text; text++ ) {
    const char *s = text, *f = part;
    while ( *f && upcase( *s ) == upcase( *f ) ) s++, f++;
    if ( *f == 0 ) return true;
  }
  return false;
}

bool FilterCriteria::matches( const Game &game ) const {
  if ( favoritesOnly && !( game.flags & GameFavorite ) ) return false;
  if ( noMetadataOnly && !( game.flags & GameNoMetadata ) ) return false;
  if ( !matchField( game, FieldGenre, genre ) || !matchField( game, FieldYear, year ) ||
       !matchField( game, FieldPlayers, players ) )
    return false;
  if ( searchLen == 0 ) return true;
  return contains( game.title, search ) || contains( game.folder, search ) ||
         contains( game.field( FieldDesc ), search );
}
