/*
   browser.cpp - the game list, the detail view, the file scan.
*/

#include <dos.h>
#include <string.h>

#include "gmenu.h"

char FNames[MAXF][FSZ];
const char *FPtrs[MAXF];
int FCount = 0;
static int FSel, FTop;
static unsigned RSeed = 0;

/* The header: "Games  (\COLLECT)", the match count and the active filters. */
static void buildGameHeader( void ) {
  char *d = HdrBuf;
  strcpy( d, HdrGames );
  if ( filtersActive( ) || FLen ) {
    d = HdrBuf + strlen( HdrBuf );
    strcpy( d, "  Found: " );
    d = numStr( d + strlen( d ), VCount );
    strcpy( d, " of " );
    numStr( d + strlen( d ), GCount );
  }
  if ( SortMode ) {             /* a sort order other than title */
    strcat( HdrBuf, "  Sort: " );
    strcat( HdrBuf, SortNames[SortMode] );
  }
  if ( FavOnly ) strcat( HdrBuf, "  Favorites" );
  if ( NoIni ) strcat( HdrBuf, "  No METADATA.INI" );
  if ( FGenre[0] ) {
    strcat( HdrBuf, "  Genre: " );
    strcat( HdrBuf, FGenre );
  }
  if ( FYear[0] ) {
    strcat( HdrBuf, "  Year: " );
    strcat( HdrBuf, FYear );
  }
  if ( FPlayers[0] ) {
    strcat( HdrBuf, "  Players: " );
    strcat( HdrBuf, FPlayers );
  }
}

/* The header: "Title  (FOLDER)"; DirBuf: the full folder path. */
static void buildFileHeader( void ) {
  strcpy( HdrBuf, Games[CurGame].title );
  strcat( HdrBuf, "  (" );
  strcat( HdrBuf, Games[CurGame].folder );
  strcat( HdrBuf, ")" );
  strcpy( putColDir( DirBuf ), Games[CurGame].folder );
}

/* The selected game, or -1 if the list is empty. */
static int curGame( void ) {
  return VCount ? VPtrs[GSel] : -1;
}

/* A random game of the list (not the current one), centered. */
static void randomGame( void ) {
  unsigned g;
  if ( VCount <= 1 ) return;
  do {
    RSeed = RSeed * 25173u + 13849u + ticks( );   /* LCG, stirred with the timer */
    g = (unsigned)( ( (unsigned long)RSeed * VCount ) >> 16 );
  } while ( g == GSel );
  GSel = g;
  GTop = GSel - GROWS / 2;
  if ( GTop < 0 ) GTop = 0;
}

static void gameKey( unsigned k ) {
  unsigned char al = (unsigned char)k;
  int g = curGame( );
  if ( k == K_DBLCLICK || al == 10 ) {          /* double click, Ctrl+Enter: play */
    if ( g >= 0 ) play( g );
    return;
  }
  if ( al != 0 && al != 0xE0 ) {
    if ( al == 0x13 ) {                         /* Ctrl+S: setup */
      if ( g >= 0 ) setup( g );
    }
    else if ( al == 5 ) {                       /* Ctrl+E: METADATA.INI */
      if ( g >= 0 ) editIni( g );
    }
    else if ( al == 8 ) filterBack( );
    else if ( al >= ' ' ) filterAdd( al );
    return;
  }
  switch ( k >> 8 ) {
    case 0x3B:                                  /* F1: help */
      showHelp( );
      break;
    case 0x3C:                                  /* F2, F3, F4: filters */
      pickFilter( F_GENRE, FGenre, "Filter by genre" );
      break;
    case 0x3D:
      pickFilter( F_YEAR, FYear, "Filter by year" );
      break;
    case 0x3E:
      pickFilter( F_PLAYERS, FPlayers, "Filter by players" );
      break;
    case 0x3F:                                  /* F5: clear the filters */
      clearFilters( );
      break;
    case 0x40:                                  /* F6: favorite; the position is kept */
      if ( g < 0 ) break;                       /*   (the game may drop out of the list) */
      toggleFav( g );
      g = GSel;
      applyFilter( );
      GSel = g;
      break;
    case 0x41:                                  /* F7: favorites only */
      FavOnly ^= 1;
      refilter( );
      break;
    case 0x5A:                                  /* Shift+F7: no METADATA.INI only */
      NoIni ^= 1;
      refilter( );
      break;
    case 0x42:                                  /* F8: recently played */
      if ( pickRecent( &g ) ) detail( g );
      break;
    case 0x43:                                  /* F9: sort order */
      if ( ++SortMode >= NSORT ) SortMode = 0;
      sortGames( );
      refilter( );
      break;
    case 0x44:                                  /* F10: random game */
      randomGame( );
      break;
  }
}

void gameLoop( void ) {
  unsigned k;
  for ( ;; ) {
    ListMode = 0;
    L.count = VCount;
    L.row0 = 2;
    L.rows = GROWS;
    buildGameHeader( );
    L.header = HdrBuf;
    L.sel = GSel;
    L.top = GTop;
    int r = runList( &k );
    GSel = L.sel;
    GTop = L.top;
    if ( r == L_SELECTED ) detail( VPtrs[L.sel] );
    else if ( r == L_KEY ) gameKey( k );
    else if ( FLen ) filterClear( );            /* Esc: the search, then the filters, */
    else if ( filtersActive( ) ) clearFilters( ); /*   then quit */
    else quit( );
  }
}


/* ---------------------------------------------------------------- detail view */

void detail( int g ) {
  unsigned k;
  int r, d;
  CurGame = g;
  DScroll = 0;
  buildFileHeader( );
rescan:
  scanFiles( );
  FSel = findRunFile( );        /* the RUN file is selected first */
  FTop = 0;
  for ( ;; ) {
    ListMode = 1;
    L.count = FCount;
    L.header = HdrBuf;
    L.row0 = 2;
    L.rows = FROWS;
    L.sel = FSel;
    L.top = FTop;
    r = runList( &k );
    FSel = L.sel;
    FTop = L.top;
    if ( r == L_ESC ) return;
    if ( r == L_SELECTED ) {
      if ( isExec( FPtrs[L.sel] ) ) {
        fileCmd( FPtrs[L.sel] );
        launch( );
      }
      beep( );                  /* not an executable file */
      continue;
    }
    if ( ( k >> 8 ) == 0x0F ) { /* Tab: executables / all files */
      ShowAll ^= 1;
      goto rescan;
    }
    /* scrolling the description: the wheel over the left box, Ctrl+PgUp/PgDn,
       Shift+PgUp/PgDn */
    if ( k == K_WHEELUP ) d = -WSTEP;
    else if ( k == K_WHEELDN ) d = WSTEP;
    else if ( ( k & 0xFF ) == 5 ) {             /* Ctrl+E: METADATA.INI */
      editIni( CurGame );
      continue;
    }
    else if ( ( k & 0xFF ) == 0 || ( k & 0xFF ) == 0xE0 ) {
      switch ( k >> 8 ) {
        case 0x40:              /* F6: favorite; the game list may change */
          toggleFav( CurGame );
          applyFilter( );
          continue;
        case 0x3B:              /* F1: help */
          showHelp( );
          continue;
        case 0x84:
        case 0x49:
          d = -( DROWS - 1 );
          break;
        case 0x76:
        case 0x51:
          d = DROWS - 1;
          break;
        default:
          continue;
      }
    }
    else continue;
    d += DScroll;               /* 0..DLines-DROWS */
    if ( d > DLines - DROWS ) d = DLines - DROWS;
    if ( d < 0 ) d = 0;
    DScroll = d;
  }
}

/* The files of the game's folder (only the executables, unless ShowAll), sorted. */
void scanFiles( void ) {
  struct find_t f;
  FCount = 0;
  char *d = putColDir( PathBuf );
  strcpy( d, Games[CurGame].folder );
  strcat( d, "\\*.*" );
  if ( _dos_findfirst( PathBuf, _A_NORMAL, &f ) == 0 ) do {
    if ( !ShowAll && !isExec( f.name ) ) continue;
    if ( FCount >= MAXF ) break;
    memcpy( FNames[FCount], f.name, FSZ );
    FPtrs[FCount] = FNames[FCount];
    FCount++;
  } while ( _dos_findnext( &f ) == 0 );
  shellSort( FPtrs, FCount, cmpi );
}
