/*
   cache.cpp - GMENU.IDX: the data read from METADATA.INI / NAME.TXT, kept between runs.

   A game's cached data is used when the date, time and size of its METADATA.INI (and of
   NAME.TXT, if the title came from there) are unchanged; otherwise the files are read
   again and the whole cache is rewritten after the scan.
   GMENU.IDX: 'GI', IDXVER, game directory (66), then per game in directory order:
     folder (13), stamps (16: METADATA.INI, NAME.TXT: time, date, size), source (1:
     1 = the title came from NAME.TXT / folder name), title length (1), title,
     record length (2), record
*/

#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "gmenu.h"

#define IDXVER 1
#define STSZ   16               /* stamps per game */
#define IHSZ   ( FSZ + STSZ + 2 ) /* entry header */

static int IdxH = -1;
static char IdxDirty;
static unsigned char IhBuf[70];

static void idxClose( void ) {
  if ( IdxH >= 0 ) close( IdxH );
  IdxH = -1;
}

/* Stop using the cache: it is rewritten after the scan. */
static void idxDrop( void ) {
  IdxDirty = 1;
  idxClose( );
}

/* 1 if all the bytes were read; otherwise the cache is dropped. */
static int idxRead( void *buf, unsigned n ) {
  if ( IdxH < 0 ) return 0;
  if ( (unsigned)read( IdxH, buf, n ) == n ) return 1;
  idxDrop( );
  return 0;
}

/* Before the scan: GMENU.IDX is opened if it is valid. */
void idxOpen( void ) {
  IdxDirty = 1;
  IdxH = open( progFile( "GMENU.IDX" ), O_RDONLY | O_BINARY );
  if ( IdxH < 0 ) return;
  IdxDirty = 0;
  if ( !idxRead( IhBuf, 3 + sizeof( ColDir ) ) ) return;
  if ( IhBuf[0] != 'G' || IhBuf[1] != 'I' || IhBuf[2] != IDXVER || memcmp( IhBuf + 3, ColDir, sizeof( ColDir ) ) ) {
    idxDrop( );
  }
}

/* name = '\NAME.EXT' -> its time, date and size (8 bytes); zeros if it is missing. */
void fileStamp( int g, const char *name, unsigned char *out ) {
  struct find_t f;
  if ( _dos_findfirst( gamePath( g, name ), _A_HIDDEN | _A_SYSTEM, &f ) ) {
    memset( out, 0, 8 );
    return;
  }
  out[0] = (unsigned char)f.wr_time;
  out[1] = (unsigned char)( f.wr_time >> 8 );
  out[2] = (unsigned char)f.wr_date;
  out[3] = (unsigned char)( f.wr_date >> 8 );
  for ( int i = 0; i < 4; i++ ) out[4 + i] = (unsigned char)( f.size >> ( 8 * i ) );
}

/* 1 if the game's title and record came from GMENU.IDX (Stamp has the METADATA.INI
   stamp; on a hit the NAME.TXT stamp is completed). */
int idxLookup( int g ) {
  Game &G = Games[g];
  unsigned len;
  if ( !idxRead( IhBuf, IHSZ ) ) return 0;
  if ( cmpi( (char *)IhBuf, G.folder ) || memcmp( IhBuf + FSZ, Stamp, 8 ) ) goto bad;
  GSrc = IhBuf[FSZ + STSZ];
  if ( GSrc ) {                 /* title from NAME.TXT: it must be unchanged too */
    fileStamp( g, "\\NAME.TXT", Stamp + 8 );
    if ( memcmp( IhBuf + FSZ + 8, Stamp + 8, 8 ) ) goto bad;
  }
  len = IhBuf[IHSZ - 1];
  if ( len > L_NAME ) goto bad;
  if ( !idxRead( G.title, len ) ) return 0;
  G.title[len] = 0;
  if ( !idxRead( &len, 2 ) ) return 0;
  if ( len ) {
    char *r = (char *)malloc( len );
    if ( r == NULL ) goto bad;
    if ( !idxRead( r, len ) ) {
      free( r );
      return 0;
    }
    G.rec = r;
  }
  return 1;
bad:
  idxDrop( );
  return 0;
}

/* The game's stamps and source, for the next GMENU.IDX. */
void keepStamp( int g ) {
  memcpy( Games[g].stamp, Stamp, STSZ );
  if ( Stamp[2] == 0 && Stamp[3] == 0 ) Games[g].flags |= G_NOINI;   /* no METADATA.INI date */
  if ( GSrc ) Games[g].flags |= G_NAMETXT;
}

static void idxWrite( void ) {
  int h = open( progFile( "GMENU.IDX" ), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE );
  if ( h < 0 ) return;
  unsigned char buf[3 + sizeof( ColDir )];
  buf[0] = 'G';
  buf[1] = 'I';
  buf[2] = IDXVER;
  memcpy( buf + 3, ColDir, sizeof( ColDir ) );
  write( h, buf, sizeof( buf ) );
  for ( int g = 0; g < GCount; g++ ) {
    Game &G = Games[g];
    unsigned char out[IHSZ + L_NAME + 2];
    unsigned tlen = strlen( G.title );
    unsigned rlen = 0;
    memcpy( out, G.folder, FSZ );
    memcpy( out + FSZ, G.stamp, STSZ );
    out[FSZ + STSZ] = ( G.flags & G_NAMETXT ) ? 1 : 0;
    out[IHSZ - 1] = (unsigned char)tlen;
    memcpy( out + IHSZ, G.title, tlen );
    if ( G.rec ) {
      for ( int f = 0; f < NKEYS - 1; f++ ) rlen += strlen( G.rec + rlen ) + 1;
    }
    out[IHSZ + tlen] = (unsigned char)rlen;
    out[IHSZ + tlen + 1] = (unsigned char)( rlen >> 8 );
    write( h, out, IHSZ + tlen + 2 );
    if ( rlen ) write( h, G.rec, rlen );
  }
  close( h );
}

/* After the scan: data past the last entry (a removed game) also needs a rewrite. */
void idxFinish( void ) {
  if ( IdxH >= 0 ) {
    if ( read( IdxH, IhBuf, 1 ) != 0 ) IdxDirty = 1;
    idxClose( );
  }
  if ( IdxDirty ) idxWrite( );
}
