/*
   stats.cpp - play statistics (GMENU.STA): how many times and when a game was started.

   GMENU.STA: records of folder (13), starts (2), date of the last start (2, DOS format);
   the records of folders missing from the game directory are kept.
*/

#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "gmenu.h"

#define STREC  17
#define MAXSTA 1000

static unsigned char *Sta;      /* the file's records; a known game's folder is cleared */
static int StaN = 0;

static unsigned getWord( const unsigned char *p ) {
  return p[0] | ( p[1] << 8 );
}

static void putWord( unsigned char *p, unsigned w ) {
  p[0] = (unsigned char)w;
  p[1] = (unsigned char)( w >> 8 );
}

void loadStats( void ) {
  Sta = (unsigned char *)malloc( STREC * MAXSTA );
  if ( Sta == NULL ) return;
  int n = readFile( progFile( "GMENU.STA" ), Sta, STREC * MAXSTA );
  if ( n < 0 ) return;
  StaN = n / STREC;
  for ( int i = 0; i < StaN; i++ ) {
    unsigned char *r = Sta + i * STREC;
    char name[FSZ];
    memcpy( name, r, FSZ );
    name[FSZ - 1] = 0;
    int g = findEntry( name );
    if ( g < 0 ) continue;
    Games[g].plays = getWord( r + FSZ );
    Games[g].last = getWord( r + FSZ + 2 );
    r[0] = 0;
  }
}

static void saveStats( void ) {
  int h = open( progFile( "GMENU.STA" ), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE );
  if ( h < 0 ) return;
  for ( int g = 0; g < GCount; g++ ) {
    unsigned char r[STREC];
    if ( Games[g].plays == 0 ) continue;
    memcpy( r, Games[g].folder, FSZ );
    putWord( r + FSZ, Games[g].plays );
    putWord( r + FSZ + 2, Games[g].last );
    write( h, r, STREC );
  }
  for ( int i = 0; i < StaN; i++ ) {         /* records of other folders */
    if ( Sta[i * STREC] ) write( h, Sta + i * STREC, STREC );
  }
  close( h );
}

/* One more start of the game, today. */
void addPlay( int g ) {
  struct dosdate_t d;
  _dos_getdate( &d );
  Games[g].plays++;
  Games[g].last = ( ( d.year - 1980 ) << 9 ) | ( d.month << 5 ) | d.day;
  saveStats( );
}

static char *num2( char *d, unsigned n ) {
  *d++ = (char)( '0' + n / 10 );
  *d++ = (char)( '0' + n % 10 );
  return d;
}

/* "5x, last 2026-10-09" or "never" */
const char *playedStr( int g ) {
  static char out[32];
  unsigned last = Games[g].last;
  if ( Games[g].plays == 0 ) return "never";
  char *d = numStr( out, Games[g].plays );
  strcpy( d, "x, last " );
  d = numStr( d + strlen( d ), ( last >> 9 ) + 1980 );
  *d++ = '-';
  d = num2( d, ( last >> 5 ) & 15 );
  *d++ = '-';
  d = num2( d, last & 31 );
  *d = 0;
  return out;
}

/* Most starts first, then the latest start, then the title. */
int cmpPlays( int a, int b ) {
  if ( Games[a].plays != Games[b].plays ) return Games[a].plays > Games[b].plays ? -1 : 1;
  if ( Games[a].last != Games[b].last ) return Games[a].last > Games[b].last ? -1 : 1;
  return cmpTitle( a, b );
}
