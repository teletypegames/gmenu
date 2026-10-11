/*
   catalog.cpp - game scanning, METADATA.INI parsing, field access.

   Per folder: METADATA.INI (NAME, GENRE, YEAR, PLAYERS, DESCRIPTION, RUN, SETUP, ARGS),
   if missing: NAME.TXT. The fields after NAME are kept in a record allocated per game.
*/

#include <dos.h>
#include <stdlib.h>
#include <string.h>

#include "gmenu.h"

Game *Games;
int GCount = 0;
int GPtrs[MAXG];
char PathBuf[128];
unsigned char Stamp[16];        /* the stamps of the game being read */
int GSrc;                       /* 1: its title came from NAME.TXT / the folder name */

static char IBuf[IBUFSZ];
static const char *const KeyTab[NKEYS] = {
  "NAME", "GENRE", "YEAR", "PLAYERS", "DESCRIPTION", "RUN", "SETUP", "ARGS"
};
static const int KvLim[NKEYS] = { L_NAME, L_GENRE, L_YEAR, L_PLAYERS, L_DESC, L_CMD, L_CMD, L_CMD };
static const char *KvOff[NKEYS];
static int KvLen[NKEYS];
static char DescBuf[L_DESC + 2];

static void readGame( int g );

void scanGames( void ) {
  struct find_t f;
  idxOpen( );
  strcpy( putColDir( PathBuf ), "*.*" );
  if ( _dos_findfirst( PathBuf, _A_SUBDIR, &f ) == 0 ) do {
    if ( !( f.attrib & _A_SUBDIR ) || f.name[0] == '.' ) continue;
    if ( GCount >= MAXG ) break;
    int g = GCount++;
    GPtrs[g] = g;
    memcpy( Games[g].folder, f.name, FSZ );
    readGame( g );
  } while ( _dos_findnext( &f ) == 0 );
  idxFinish( );
}

char *putColDir( char *d ) {
  strcpy( d, ColDir );
  d += strlen( d );
  *d++ = '\\';
  *d = 0;
  return d;
}

/* name = '\X.Y' -> PathBuf = game directory\folder\X.Y */
const char *gamePath( int g, const char *name ) {
  strcpy( putColDir( PathBuf ), Games[g].folder );
  strcat( PathBuf, name );
  return PathBuf;
}

/* name = '\X.Y' -> IBuf (null-terminated, in CP437): the length, or -1 if missing */
static int openRead( int g, const char *name ) {
  int n = readFile( gamePath( g, name ), IBuf, IBUFSZ - 1 );
  if ( n < 0 ) return -1;
  IBuf[n] = 0;
  return utf8To437( IBuf );
}

static void defaultTitle( int g ) {
  strcpy( Games[g].title, Games[g].folder );
}

/* The first line of the text becomes the title (max 50 characters, without leading and
   trailing spaces); if it is empty, the title remains the folder name. */
static void setTitle( int g, const char *s, int len ) {
  char *d = Games[g].title;
  int n = 0;
  while ( len-- > 0 ) {
    char c = *s++;
    if ( isEol( c ) ) break;
    if ( c == 9 ) c = ' ';
    if ( c == ' ' && n == 0 ) continue;
    if ( n >= L_NAME ) break;
    d[n++] = c;
  }
  while ( n && d[n - 1] == ' ' ) n--;
  if ( n == 0 ) defaultTitle( g );
  else d[n] = 0;
}

static void parseGame( int g );

/* The title and the data from GMENU.IDX if the files did not change, otherwise from
   METADATA.INI, if missing: the title from NAME.TXT. */
static void readGame( int g ) {
  GSrc = 0;
  defaultTitle( g );
  fileStamp( g, "\\METADATA.INI", Stamp );
  memset( Stamp + 8, 0, 8 );
  if ( !idxLookup( g ) ) {
    Games[g].rec = NULL;
    GSrc = 0;
    defaultTitle( g );
    parseGame( g );
  }
  keepStamp( g );
}


/* ---------------------------------------------------------------- METADATA.INI */

static int findKey( const char *beg, const char *end ) {
  for ( int k = 0; k < NKEYS; k++ ) {
    const char *s = beg, *t = KeyTab[k];
    while ( s != end && upcase( *s ) == *t ) s++, t++;
    if ( s == end && *t == 0 ) return k;
  }
  return -1;
}

/* A DESCRIPTION value -> appended to DescBuf: "\n" = line break, "\\" = backslash, each
   further DESCRIPTION= line starts a new line. */
static void descAppend( const char *v, int len ) {
  char *d = DescBuf + KvLen[4];
  char *lim = DescBuf + L_DESC;
  KvOff[4] = DescBuf;
  if ( KvLen[4] ) {
    if ( d >= lim ) return;
    *d++ = 10;
  }
  while ( len-- > 0 ) {
    char c = *v++;
    if ( c == '\\' && len > 0 ) {
      if ( *v == '\\' ) v++, len--;
      else if ( ( *v | 0x20 ) == 'n' ) c = 10, v++, len--;
    }
    if ( d >= lim ) break;
    *d++ = c;
  }
  KvLen[4] = d - DescBuf;
}

/* IBuf -> KvOff / KvLen (keys of KeyTab); missing: length 0. "KEY = value" lines, case
   insensitive; lines starting with ';' '#' '[' are skipped. */
static void parseIni( void ) {
  const char *s = skipBom( IBuf );
  memset( KvLen, 0, sizeof( KvLen ) );
  for ( ;; ) {
    s = skipBlank( s );
    char c = *s;
    if ( isEol( c ) ) {
      if ( c == 0 || c == 0x1A ) break;
      s++;
      continue;
    }
    if ( c == ';' || c == '#' || c == '[' ) {
      while ( !isEol( *s ) ) s++;
      continue;
    }
    const char *key = s;
    while ( !isEol( *s ) && *s != '=' ) s++;
    if ( *s != '=' ) continue;
    int k = findKey( key, trimBack( key, s ) );
    const char *v = skipBlank( s + 1 );
    for ( s = v; !isEol( *s ); s++ );
    const char *e = trimBack( v, s );
    int len = e - v;
    if ( len >= 2 && v[0] == '"' && e[-1] == '"' ) {
      v++;
      len -= 2;
    }
    if ( k < 0 ) continue;
    if ( k == 4 ) descAppend( v, len );
    else {
      KvOff[k] = v;
      KvLen[k] = len;
    }
  }
}

/* GENRE..ARGS -> a new record of the game (none when they are all empty or there is no
   memory). */
static void storeRecord( int g ) {
  unsigned sum = 0;
  for ( int k = 0; k < NKEYS; k++ ) {
    if ( KvLen[k] > KvLim[k] ) KvLen[k] = KvLim[k];
    if ( k ) sum += KvLen[k];
  }
  if ( sum == 0 ) return;
  char *r = (char *)malloc( sum + NKEYS - 1 );
  if ( r == NULL ) return;
  Games[g].rec = r;
  for ( int f = 1; f < NKEYS; f++ ) {
    const char *s = KvOff[f];
    for ( int n = KvLen[f]; n; n-- ) {
      char c = *s++;
      *r++ = ( c == 9 ) ? ' ' : c;
    }
    *r++ = 0;
  }
}

static void parseGame( int g ) {
  if ( openRead( g, "\\METADATA.INI" ) >= 0 ) {
    parseIni( );
    storeRecord( g );
    if ( KvLen[0] ) {
      setTitle( g, KvOff[0], KvLen[0] );
      return;
    }
  }
  GSrc = 1;
  fileStamp( g, "\\NAME.TXT", Stamp + 8 );
  int n = openRead( g, "\\NAME.TXT" );
  if ( n < 0 ) return;
  const char *s = skipBom( IBuf );
  setTitle( g, s, n - ( s - IBuf ) );
}

/* A field of the game's record; empty if it has none. */
const char *getField( int g, int field ) {
  const char *s = Games[g].rec;
  if ( s == NULL ) return "";
  while ( field-- ) s += strlen( s ) + 1;
  return s;
}
