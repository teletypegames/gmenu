/*
   GMENUPRG.EXE - game menu for the \COLLECT folder (DOS / DOSBox)
   Build: build.sh (Open Watcom)
   Launch: GMENU.BAT -> GMENUPRG.EXE /B [/A] [game_directory]  (default: \COLLECT)
           /A = all files are shown in the game's folder, not just executables
   Exit code: 0 = GMENURUN.BAT created next to the program (must be run), 1 = quit, 2 = error
   The program's files (GMENURUN.BAT, GMENU.SEL/.FAV/.STA/.IDX) are in its own folder.
*/

#include <dos.h>
#include <i86.h>
#include <stdlib.h>
#include <string.h>

#include "gmenu.h"

#ifndef COLLECT
#define COLLECT "\\COLLECT"     /* default game directory (build.sh: gmenu.ini collect_dir) */
#endif

char ColDir[66] = COLLECT;
const char *CdPath;             /* ColDir without the drive, for cd */
char GDrive, HDrive;            /* the drive of the games, of the start */
char HdrGames[96];              /* "Games  (directory)" */
char HdrBuf[200];               /* the header of the list */
char DirBuf[80];                /* the folder of the game in the detail view */
static char ArgB = 0, ArgA = 0;

/* GMENUPRG.EXE /B [/A] [directory]   e.g. /B \COLLECT  or  /B /A D:\GAMES */
static void parseArgs( int argc, char *argv[] ) {
  char arg[64] = "";
  for ( int i = 1; i < argc; i++ ) {
    const char *a = argv[i];
    if ( a[0] == '/' ) {
      char c = upcase( a[1] );
      if ( c == 'B' ) ArgB = 1;
      if ( c == 'A' ) ArgA = 1;
      continue;
    }
    int n = 0;
    for ( ; *a && n < 60; a++ ) arg[n++] = upcase( *a );
    arg[n] = 0;
  }
  unsigned drive;
  _dos_getdrive( &drive );
  HDrive = GDrive = (char)( 'A' + drive - 1 );
  if ( arg[0] ) {               /* ColDir: [X:] + '\' + path, without a trailing '\' */
    const char *s = arg;
    char *d = ColDir;
    if ( s[1] == ':' ) {
      GDrive = s[0];
      *d++ = *s++;
      *d++ = *s++;
    }
    if ( *s != '\\' ) *d++ = '\\';
    strcpy( d, s );
    d += strlen( d );
    while ( d > ColDir && d[-1] == '\\' ) *--d = 0;
  }
  CdPath = ( ColDir[1] == ':' ) ? ColDir + 2 : ColDir;
  strcpy( HdrGames, "Games  (" );
  strcat( HdrGames, ColDir );
  strcat( HdrGames, ")" );
}

static void initMouse( void ) {
  union REGS r;
  r.w.ax = 0;
  int86( 0x33, &r, &r );
  if ( r.w.ax != 0xFFFF ) return;
  HasMouse = 1;
  r.w.ax = 0x11;                /* CuteMouse WheelAPI: wheel data in BH of function 3 */
  int86( 0x33, &r, &r );
  if ( r.w.ax == 0x574D && ( r.h.cl & 1 ) ) HasWheel = 1;
}

void quit( void ) {
  saveSel( );
  restoreScreen( );
  exit( 1 );
}

int main( int argc, char *argv[] ) {
  union REGS r;
  getProgDir( argv[0] );
  parseArgs( argc, argv );
  if ( !ArgB ) {                /* only GMENU.BAT starts the game it chose */
    say( "Start the menu with GMENU.BAT (type GMENU).\r\n" );
    return 2;
  }
  Games = (Game *)calloc( MAXG, sizeof( Game ) );
  if ( Games == NULL ) {
    say( "Not enough memory.\r\n" );
    return 2;
  }
  setTextMode( );
  r.h.ah = 1;                   /* the text cursor is hidden */
  r.w.cx = 0x2000;
  int86( 0x10, &r, &r );
  initMouse( );
  scanGames( );
  if ( GCount == 0 ) {
    restoreScreen( );
    say( "No game folders found in: " );
    say( ColDir );
    say( "\r\n" );
    return 2;
  }
  loadFavs( );
  loadStats( );
  loadSel( );
  if ( ArgA ) ShowAll = 1;
  mouseShow( );
  gameLoop( );
  return 0;
}
