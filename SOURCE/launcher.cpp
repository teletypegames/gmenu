/*
   launcher.cpp - command lines (RUN / SETUP / ARGS) and GMENURUN.BAT.

   GMENU.BAT runs GMENURUN.BAT after the menu exited with code 0, then starts the menu
   again: the menu is not in memory while a game runs.
*/

#include <dos.h>
#include <stdlib.h>
#include <string.h>

#include "gmenu.h"

int CurGame;
static char CmdBuf[2 * L_CMD + 2];  /* the command line that runs the file */
static char OutBuf[256];

static char *cpy( char *d, const char *s ) {
  strcpy( d, s );
  return d + strlen( d );
}

/* OutBuf..end -> GMENURUN.BAT */
static void writeBat( const char *end ) {
  writeFile( progFile( "GMENURUN.BAT" ), OutBuf, end - OutBuf );
}

static void runExit( void ) {
  saveSel( );
  restoreScreen( );
  exit( 0 );
}

/* GMENURUN.BAT: to the game's folder, the command, back to the folder of the start. */
static void writeRunner( void ) {
  char *d = cpy( OutBuf, "@echo off\r\n" );
  *d++ = GDrive;
  d = cpy( d, ":\r\ncd " );
  d = cpy( d, CdPath );
  *d++ = '\\';
  d = cpy( d, Games[CurGame].folder );
  d = cpy( d, "\r\n" );
  const char *s = CmdBuf;               /* a .BAT file is started with call */
  while ( (unsigned char)*s > ' ' && *s != '.' ) s++;
  if ( *s == '.' && upcase( s[1] ) == 'B' ) d = cpy( d, "call " );
  d = cpy( d, CmdBuf );
  d = cpy( d, "\r\n" );
  *d++ = HDrive;
  d = cpy( d, ":\r\ncd \\" );
  d = cpy( d, StartDir );
  d = cpy( d, "\r\n" );
  writeBat( d );
}

/* CurGame, CmdBuf -> GMENURUN.BAT, the play statistics, the recent list, GMENU.SEL;
   exit with code 0. */
void launch( void ) {
  writeRunner( );
  addPlay( CurGame );
  addRecent( CurGame );
  runExit( );
}

/* GMENURUN.BAT runs GMEDIT.EXE (next to this program) for the game's folder, which
   creates or edits its METADATA.INI; a beep if there is no GMEDIT.EXE. */
void editIni( int g ) {
  struct find_t f;
  char editor[96];
  CurGame = g;
  strcpy( editor, progFile( "GMEDIT.EXE" ) );
  if ( _dos_findfirst( editor, _A_NORMAL, &f ) ) {
    beep( );
    return;
  }
  char *d = cpy( OutBuf, "@echo off\r\n" );
  d = cpy( d, editor );
  d = cpy( d, " /B " );
  *d++ = GDrive;
  *d++ = ':';
  d = cpy( d, CdPath );
  *d++ = '\\';
  d = cpy( d, Games[g].folder );
  d = cpy( d, "\r\n" );
  writeBat( d );
  runExit( );
}

/* ' ' + ARGS, if the game has them */
static void addArgs( void ) {
  const char *args = getField( CurGame, F_ARGS );
  if ( !*args ) return;
  strcat( CmdBuf, " " );
  strcat( CmdBuf, args );
}

/* The first word of the command line is the file name (case insensitive). */
int tokEq( const char *cmd, const char *name ) {
  for ( ;; ) {
    char c = ( (unsigned char)*cmd > ' ' ) ? *cmd : 0;
    if ( upcase( c ) != upcase( *name ) ) return 0;
    if ( c == 0 ) return 1;
    cmd++, name++;
  }
}

/* A file of the game's folder -> CmdBuf: the RUN file gets RUN's arguments and ARGS, the
   SETUP file SETUP's arguments. */
void fileCmd( const char *file ) {
  const char *run = getField( CurGame, F_RUN );
  if ( tokEq( run, file ) ) {
    strcpy( CmdBuf, run );
    addArgs( );
    return;
  }
  const char *setup = getField( CurGame, F_SETUP );
  strcpy( CmdBuf, tokEq( setup, file ) ? setup : file );
}

/* The game's RUN command; without RUN its only executable; otherwise (or if there is
   none) the detail view opens. */
void play( int g ) {
  CurGame = g;
  const char *run = getField( g, F_RUN );
  if ( *run ) {
    strcpy( CmdBuf, run );
    addArgs( );
    launch( );
  }
  char all = ShowAll;
  ShowAll = 0;
  scanFiles( );
  ShowAll = all;
  if ( FCount == 1 ) {
    fileCmd( FPtrs[0] );
    launch( );
  }
  detail( g );
}

/* The game's SETUP command (a beep if it has none). */
void setup( int g ) {
  CurGame = g;
  const char *cmd = getField( g, F_SETUP );
  if ( !*cmd ) {
    beep( );
    return;
  }
  strcpy( CmdBuf, cmd );
  launch( );
}

/* The index of the RUN file in the file list (0 if it is not listed). */
int findRunFile( void ) {
  const char *run = getField( CurGame, F_RUN );
  for ( int i = 0; i < FCount; i++ ) {
    if ( tokEq( run, FPtrs[i] ) ) return i;
  }
  return 0;
}
