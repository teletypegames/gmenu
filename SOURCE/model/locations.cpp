#include <string.h>

#include "locations.h"
#include "../lib/core/str.h"

void Locations::setProgDir( const char *argv0 ) {
  const char *slash = strrchr( argv0, '\\' );
  if ( slash == NULL || slash - argv0 + 1 > (int)sizeof( progDir ) - 14 ) {
    strcpy( progDir, "\\GMENU\\" );
    return;
  }
  memcpy( progDir, argv0, slash - argv0 + 1 );
  progDir[slash - argv0 + 1] = 0;
}

void Locations::setGameDir( const char *dir ) {
  char arg[64];
  int n = 0;
  for ( ; *dir && n < 60; dir++ ) arg[n++] = upcase( *dir );
  arg[n] = 0;
  const char *s = arg;
  char *d = gameDir;
  if ( s[1] == ':' ) {
    gameDrive = s[0];
    *d++ = *s++;
    *d++ = *s++;
  }
  if ( *s != '\\' ) *d++ = '\\';
  strcpy( d, s );
  d += strlen( d );
  while ( d > gameDir && d[-1] == '\\' ) *--d = 0;
}

const char *Locations::gamePath() const {
  return gameDir[1] == ':' ? gameDir + 2 : gameDir;
}

void Locations::progFile( const char *name, char *out ) const {
  strcpy( out, progDir );
  strcat( out, name );
}

void Locations::gameFile( const char *folder, const char *name, char *out ) const {
  strcpy( out, gameDir );
  strcat( out, "\\" );
  strcat( out, folder );
  strcat( out, name );
}
