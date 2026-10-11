#include <string.h>

#include "launch_service.h"
#include "../lib/platform/clock.h"
#include "../model/file_list.h"
#include "../model/game_library.h"
#include "../model/launch_command.h"
#include "../model/session.h"
#include "../repository/game_files_repository.h"
#include "../repository/launcher_repository.h"
#include "../repository/stats_repository.h"

LaunchService::LaunchService( GameLibrary &library, Session &session, const Locations &where,
                              LauncherRepository &launcher, StatsRepository &stats, GameFilesRepository &files,
                              Clock &clock )
  : library_( library ), session_( session ), where_( where ), launcher_( launcher ), stats_( stats ), files_( files ),
    clock_( clock ) {}

void LaunchService::launch( int game, const char *command ) {
  char script[ScriptSize];
  Game &g = library_.at( game );
  launcher_.writeScript( script, runScript( where_, g, command, script ) );
  g.plays++;
  g.last = clock_.today();
  stats_.save( library_ );
  session_.recent.add( g.folder );
}

bool LaunchService::play( int game, FileList &files ) {
  char command[CommandSize];
  const Game &g = library_.at( game );
  if ( *g.field( FieldRun ) ) {
    runCommand( g, command );
    launch( game, command );
    return true;
  }
  listFiles( game, false, files );
  if ( files.count != 1 ) return false;
  fileCommand( g, files.sorted[0], command );
  launch( game, command );
  return true;
}

bool LaunchService::setup( int game ) {
  const char *command = library_.at( game ).field( FieldSetup );
  if ( !*command ) return false;
  char copy[CommandSize];
  strcpy( copy, command );
  launch( game, copy );
  return true;
}

void LaunchService::runFile( int game, const char *file ) {
  char command[CommandSize];
  fileCommand( library_.at( game ), file, command );
  launch( game, command );
}

bool LaunchService::editMetadata( int game ) {
  char editor[96], script[ScriptSize];
  if ( !launcher_.findEditor( editor ) ) return false;
  launcher_.writeScript( script, editScript( where_, library_.at( game ), editor, script ) );
  return true;
}

void LaunchService::listFiles( int game, bool all, FileList &files ) {
  files_.list( library_.at( game ).folder, all, files );
  files.sort();
}

int LaunchService::runFileIndex( int game, const FileList &files ) {
  const char *run = library_.at( game ).field( FieldRun );
  for ( int i = 0; i < files.count; i++ ) {
    if ( commandRuns( run, files.sorted[i] ) ) return i;
  }
  return 0;
}
