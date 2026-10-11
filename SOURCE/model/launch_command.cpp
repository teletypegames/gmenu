#include <string.h>

#include "game.h"
#include "launch_command.h"
#include "locations.h"
#include "../lib/core/str.h"

bool commandRuns( const char *command, const char *file ) {
  for ( ;; ) {
    char c = ( (unsigned char)*command > ' ' ) ? *command : 0;
    if ( upcase( c ) != upcase( *file ) ) return false;
    if ( c == 0 ) return true;
    command++, file++;
  }
}

static void addArgs( const Game &game, char *out ) {
  const char *args = game.field( FieldArgs );
  if ( !*args ) return;
  strcat( out, " " );
  strcat( out, args );
}

void runCommand( const Game &game, char *out ) {
  strcpy( out, game.field( FieldRun ) );
  addArgs( game, out );
}

void fileCommand( const Game &game, const char *file, char *out ) {
  if ( commandRuns( game.field( FieldRun ), file ) ) {
    runCommand( game, out );
    return;
  }
  const char *setup = game.field( FieldSetup );
  strcpy( out, commandRuns( setup, file ) ? setup : file );
}

static char *put( char *d, const char *s ) {
  strcpy( d, s );
  return d + strlen( d );
}

static bool isBatch( const char *command ) {
  const char *s = command;
  while ( (unsigned char)*s > ' ' && *s != '.' ) s++;
  return *s == '.' && upcase( s[1] ) == 'B';
}

unsigned runScript( const Locations &where, const Game &game, const char *command, char *out ) {
  char *d = put( out, "@echo off\r\n" );
  *d++ = where.gameDrive;
  d = put( d, ":\r\ncd " );
  d = put( d, where.gamePath() );
  *d++ = '\\';
  d = put( d, game.folder );
  d = put( d, "\r\n" );
  if ( isBatch( command ) ) d = put( d, "call " );
  d = put( d, command );
  d = put( d, "\r\n" );
  *d++ = where.homeDrive;
  d = put( d, ":\r\ncd \\" );
  d = put( d, where.startDir );
  d = put( d, "\r\n" );
  return d - out;
}

unsigned editScript( const Locations &where, const Game &game, const char *editor, char *out ) {
  char *d = put( out, "@echo off\r\n" );
  d = put( d, editor );
  d = put( d, " /B " );
  *d++ = where.gameDrive;
  *d++ = ':';
  d = put( d, where.gamePath() );
  *d++ = '\\';
  d = put( d, game.folder );
  d = put( d, "\r\n" );
  return d - out;
}
