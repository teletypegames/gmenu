/*
   session.cpp - the last position and filters (GMENU.SEL).

   GMENU.SEL: 'GS' + version (1) + folder name (13) + search length (1) + search (32)
              + filters: genre (25), year (11), players (11) + all-files flag (1)
              + favorites-only flag (1) + sort order (1) + without-METADATA.INI-only flag (1)
              + recently played folder names (NREC * 13)
   A file with another magic, version or size is ignored (format of an older build).
*/

#include <string.h>

#include "gmenu.h"

#define SELVER 4
#define SELSZ  ( 3 + FSZ + 1 + 32 + ( L_GENRE + 1 ) + ( L_YEAR + 1 ) + ( L_PLAYERS + 1 ) + 4 + NREC * FSZ )

static char SelName[FSZ];

/* The fields of the file: their place in the variables, their size. */
static struct { void *var; unsigned size; } const SelFields[] = {
  { SelName, FSZ }, { &FLen, 1 }, { FBuf, 32 }, { FGenre, L_GENRE + 1 }, { FYear, L_YEAR + 1 },
  { FPlayers, L_PLAYERS + 1 }, { &ShowAll, 1 }, { &FavOnly, 1 }, { &SortMode, 1 }, { &NoIni, 1 },
  { Recent, NREC * FSZ }
};
#define NSELF ( sizeof( SelFields ) / sizeof( SelFields[0] ) )

void loadSel( void ) {
  unsigned char buf[SELSZ + 1];
  int n = readFile( progFile( "GMENU.SEL" ), buf, sizeof( buf ) );
  if ( n == SELSZ && buf[0] == 'G' && buf[1] == 'S' && buf[2] == SELVER ) {
    const unsigned char *p = buf + 3;
    for ( int i = 0; i < NSELF; i++ ) {
      memcpy( SelFields[i].var, p, SelFields[i].size );
      p += SelFields[i].size;
    }
    if ( FLen > 30 ) FLen = 0;
    SelName[FSZ - 1] = 0;
    FGenre[L_GENRE] = 0;
    FYear[L_YEAR] = 0;
    FPlayers[L_PLAYERS] = 0;
    ShowAll &= 1;
    FavOnly &= 1;
    NoIni &= 1;
    for ( int r = 0; r < NREC; r++ ) Recent[r][FSZ - 1] = 0;
    if ( SortMode >= NSORT ) SortMode = 0;
  }
  sortGames( );
  FBuf[FLen] = 0;
  applyFilter( );
  for ( int i = 0; i < VCount; i++ ) {         /* the folder in the filtered list */
    if ( cmpi( Games[VPtrs[i]].folder, SelName ) == 0 ) {
      GSel = i;
      GTop = i - GROWS / 2;
      if ( GTop < 0 ) GTop = 0;
      break;
    }
  }
}

void saveSel( void ) {
  unsigned char buf[SELSZ];
  unsigned char *p = buf + 3;
  SelName[0] = 0;
  if ( GSel < VCount ) strcpy( SelName, Games[VPtrs[GSel]].folder );
  buf[0] = 'G';
  buf[1] = 'S';
  buf[2] = SELVER;
  for ( int i = 0; i < NSELF; i++ ) {
    memcpy( p, SelFields[i].var, SelFields[i].size );
    p += SelFields[i].size;
  }
  writeFile( progFile( "GMENU.SEL" ), buf, SELSZ );
}
