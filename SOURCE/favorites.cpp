/*
   favorites.cpp - favorite games (GMENU.FAV), recently played games (kept in GMENU.SEL).
*/

#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "gmenu.h"

#define FAVSZ ( MAXF * FSZ )

char Recent[NREC][FSZ];         /* folder names, the newest first */

int isFav( int g ) {
  return Games[g].flags & G_FAV;
}

/* GMENU.FAV: the folder names of the favorites, one per line. */
static void saveFavs( void ) {
  int h = open( progFile( "GMENU.FAV" ), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE );
  if ( h < 0 ) return;
  for ( int i = 0; i < GCount; i++ ) {
    if ( !isFav( GPtrs[i] ) ) continue;
    const char *name = Games[GPtrs[i]].folder;
    write( h, name, strlen( name ) );
    write( h, "\r\n", 2 );
  }
  close( h );
}

void toggleFav( int g ) {
  Games[g].flags ^= G_FAV;
  saveFavs( );
}

void loadFavs( void ) {
  char *buf = (char *)malloc( FAVSZ );
  if ( buf == NULL ) return;
  int n = readFile( progFile( "GMENU.FAV" ), buf, FAVSZ - 1 );
  if ( n > 0 ) {
    buf[n] = 0;
    for ( char *s = buf; *s; ) {
      if ( (unsigned char)*s <= ' ' ) {
        s++;
        continue;
      }
      char *name = s;
      while ( (unsigned char)*s > ' ' ) s++;
      char sep = *s;
      *s = 0;
      int g = findEntry( name );
      if ( g >= 0 ) Games[g].flags |= G_FAV;
      if ( sep == 0 ) break;
      s++;
    }
  }
  free( buf );
}

/* The game of a folder name, or -1. */
int findEntry( const char *folder ) {
  for ( int i = 0; i < GCount; i++ ) {
    if ( cmpi( Games[GPtrs[i]].folder, folder ) == 0 ) return GPtrs[i];
  }
  return -1;
}


/* ---------------------------------------------------------------- recently played */

/* The game -> to the front of the recent list (when it is not there, the last one
   drops off). */
void addRecent( int g ) {
  int i;
  for ( i = 0; i < NREC - 1 && cmpi( Recent[i], Games[g].folder ); i++ );
  for ( ; i > 0; i-- ) memcpy( Recent[i], Recent[i - 1], FSZ );
  strcpy( Recent[0], Games[g].folder );
}

/* A picker with the recently played games -> g; 0 if cancelled / empty. */
int pickRecent( int *g ) {
  int games[NREC];
  unsigned key;
  int r;
  PCount = 0;
  for ( int i = 0; i < NREC; i++ ) {
    if ( !Recent[i][0] ) continue;
    int e = findEntry( Recent[i] );
    if ( e < 0 ) continue;
    games[PCount] = e;
    PPtrs[PCount++] = Games[e].title;
  }
  if ( PCount == 0 ) {
    beep( );
    return 0;
  }
  PLeft = 0x0A;
  PRight = 0x45;
  PTitle = "Recently played";
  ListMode = 2;
  L.count = PCount;
  L.header = PTitle;
  L.row0 = 2;
  L.rows = PROWS;
  L.sel = 0;
  L.top = 0;
  while ( ( r = runList( &key ) ) == L_KEY );
  if ( r == L_ESC ) return 0;
  *g = games[L.sel];
  Keep = *g;                    /* selected in the game list too, if it is visible */
  findKeep( );
  return 1;
}
