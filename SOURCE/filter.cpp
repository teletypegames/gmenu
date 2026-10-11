/*
   filter.cpp - search, genre / year / players filters, the filter picker.
*/

#include <string.h>

#include "gmenu.h"

int VPtrs[MAXG];
int VCount = 0, GSel = 0, GTop = 0;
char FBuf[32];
unsigned char FLen = 0;
char FGenre[L_GENRE + 1], FYear[L_YEAR + 1], FPlayers[L_PLAYERS + 1];
char FavOnly = 0, NoIni = 0;    /* only the favorites, only the games without METADATA.INI */
int Keep;

const char *PPtrs[MAXP];        /* picker values */
int PCount;
int PLeft, PRight;              /* picker box columns */
const char *PTitle;
static char Pool[POOLSZ];
static char *PPool;

void filterAdd( char c ) {
  if ( FLen >= 30 ) return;
  FBuf[FLen++] = c;
  FBuf[FLen] = 0;
  refilter( );
}

void filterBack( void ) {
  if ( FLen == 0 ) return;
  FBuf[--FLen] = 0;
  refilter( );
}

int filtersActive( void ) {
  return FGenre[0] || FYear[0] || FPlayers[0] || FavOnly || NoIni;
}

void clearFilters( void ) {
  FGenre[0] = FYear[0] = FPlayers[0] = 0;
  FavOnly = NoIni = 0;
  refilter( );
}

void filterClear( void ) {
  FLen = 0;
  FBuf[0] = 0;
  refilter( );
}

/* Filter again, keeping the selected game if it is still visible (centered). */
void refilter( void ) {
  Keep = VCount ? VPtrs[GSel] : -1;
  applyFilter( );
  GSel = 0;
  findKeep( );
  GTop = GSel - GROWS / 2;
  if ( GTop < 0 ) GTop = 0;
}

/* The Keep game in VPtrs -> GSel (unchanged if it is not there). */
void findKeep( void ) {
  for ( int i = 0; i < VCount; i++ ) {
    if ( VPtrs[i] == Keep ) {
      GSel = i;
      return;
    }
  }
}

/* text contains FBuf (case insensitive) */
static int inStr( const char *text ) {
  for ( ; *text; text++ ) {
    const char *s = text, *f = FBuf;
    while ( *f && upcase( *s ) == upcase( *f ) ) s++, f++;
    if ( *f == 0 ) return 1;
  }
  return 0;
}

int parseNum( const char **s, unsigned *n ) {
  const char *p = *s;
  *n = 0;
  while ( *p >= '0' && *p <= '9' ) *n = *n * 10 + ( *p++ - '0' );
  if ( p == *s ) return 0;
  *s = p;
  return 1;
}

/* "a", "a-b" or "a+" -> a, b ("a+": FFFFh); 0 if it is not a number */
int parseRange( const char **s, unsigned *a, unsigned *b ) {
  if ( !parseNum( s, a ) ) return 0;
  *b = *a;
  *s = skipBlank( *s );
  if ( **s == '+' ) *b = 0xFFFF;
  else if ( **s == '-' ) {
    *s = skipBlank( *s + 1 );
    if ( !parseNum( s, b ) ) *b = *a;
  }
  return 1;
}

static int isDecade( const char *filter ) {
  return strlen( filter ) == 5 && ( filter[4] | 0x20 ) == 's';
}

/* An empty filter or a match. YEAR: "1990s" matches 1990..1999; PLAYERS: a number
   matches the "a", "a-b" and "a+" ranges that contain it. */
static int matchField( int g, int field, const char *filter ) {
  if ( !*filter ) return 1;
  const char *v = getField( g, field );
  if ( field == F_PLAYERS ) {
    const char *p = filter;
    unsigned n, a, b;
    if ( parseNum( &p, &n ) ) {
      p = v;
      if ( parseRange( &p, &a, &b ) ) return n >= a && n <= b;
    }
  }
  else if ( field == F_YEAR && isDecade( filter ) ) {
    return v[0] == filter[0] && v[1] == filter[1] && v[2] == filter[2] && v[3] >= '0' && v[3] <= '9';
  }
  return cmpi( v, filter ) == 0;
}

/* Filters: favorites only, without METADATA.INI only, GENRE / YEAR / PLAYERS; search:
   title, folder name or description. */
static int matches( int g ) {
  Game &G = Games[g];
  if ( FavOnly && !( G.flags & G_FAV ) ) return 0;
  if ( NoIni && !( G.flags & G_NOINI ) ) return 0;
  if ( !matchField( g, F_GENRE, FGenre ) || !matchField( g, F_YEAR, FYear ) ||
       !matchField( g, F_PLAYERS, FPlayers ) ) return 0;
  if ( FLen == 0 ) return 1;
  return inStr( G.title ) || inStr( G.folder ) || inStr( getField( g, F_DESC ) );
}

/* GPtrs -> VPtrs according to the filters and the search */
void applyFilter( void ) {
  VCount = 0;
  for ( int i = 0; i < GCount; i++ ) {
    if ( matches( GPtrs[i] ) ) VPtrs[VCount++] = GPtrs[i];
  }
}


/* ---------------------------------------------------------------- picker */

static void poolAdd( const char *s ) {
  if ( PCount >= MAXP || PPool > Pool + POOLSZ - L_GENRE - 1 ) return;
  PPtrs[PCount++] = PPool;
  strcpy( PPool, s );
  PPool += strlen( s ) + 1;
}

static int cmpStr( const char *a, const char *b ) {
  return cmpi( a, b );
}

/* PLAYERS: every player count 1..MAXPL covered by a game's range ("a+": only a). */
static void collectPlayers( void ) {
  char seen[MAXPL + 1];
  memset( seen, 0, sizeof( seen ) );
  for ( int i = 0; i < GCount; i++ ) {
    const char *p = getField( GPtrs[i], F_PLAYERS );
    unsigned a, b;
    if ( !parseRange( &p, &a, &b ) ) continue;
    if ( b == 0xFFFF ) b = a;
    if ( b > MAXPL ) b = MAXPL;
    for ( ; a <= b; a++ ) seen[a] = 1;
  }
  for ( int n = 1; n <= MAXPL; n++ ) {
    char num[6];
    if ( !seen[n] ) continue;
    numStr( num, n );
    poolAdd( num );
  }
}

static int isYear4( const char *s ) {
  for ( int i = 0; i < 4; i++ ) {
    if ( s[i] < '0' || s[i] > '9' ) return 0;
  }
  return 1;
}

/* YEAR: the years are PPtrs[1..PCount-1], sorted -> the decades ("1990s") are added and
   moved in front of the years. */
static void addDecades( void ) {
  int years = PCount;
  for ( int i = 1; i < years; i++ ) {
    const char *s = PPtrs[i];
    if ( !isYear4( s ) ) continue;
    char dec[6];
    dec[0] = s[0];
    dec[1] = s[1];
    dec[2] = s[2];
    dec[3] = '0';
    dec[4] = 's';
    dec[5] = 0;
    if ( PCount == years || cmpi( PPtrs[PCount - 1], dec ) ) poolAdd( dec );
  }
  for ( int j = years, k = 1; j < PCount; j++, k++ ) {
    const char *d = PPtrs[j];
    memmove( &PPtrs[k + 1], &PPtrs[k], ( j - k ) * sizeof( PPtrs[0] ) );
    PPtrs[k] = d;
  }
}

/* Pick a filter (dest) from the values the games have. */
void pickFilter( int field, char *dest, const char *title ) {
  unsigned key;
  int r;
  PPtrs[0] = "(all)";
  PCount = 1;
  PPool = Pool;
  if ( field == F_PLAYERS ) collectPlayers( );
  else {
    for ( int i = 0; i < GCount; i++ ) {
      const char *v = getField( GPtrs[i], field );
      if ( !*v ) continue;
      int k;
      for ( k = 1; k < PCount && cmpi( v, PPtrs[k] ); k++ );
      if ( k == PCount ) poolAdd( v );
    }
    shellSort( PPtrs + 1, PCount - 1, cmpStr );
    if ( field == F_YEAR ) addDecades( );
  }
  L.sel = 0;                    /* the current value is selected */
  if ( *dest ) {
    for ( int k = 1; k < PCount; k++ ) {
      if ( cmpi( dest, PPtrs[k] ) == 0 ) {
        L.sel = k;
        break;
      }
    }
  }
  PLeft = 0x18;
  PRight = 0x37;
  PTitle = title;
  ListMode = 2;
  L.count = PCount;
  L.header = title;
  L.row0 = 2;
  L.rows = PROWS;
  L.top = 0;
  while ( ( r = runList( &key ) ) == L_KEY );
  if ( r == L_ESC ) return;
  if ( L.sel == 0 ) dest[0] = 0;
  else strcpy( dest, PPtrs[L.sel] );
  refilter( );
}
