#include <stdlib.h>
#include <string.h>

#include "../../lib/testing/host/minitest.h"
#include "../../model/executable_list.h"
#include "../../model/filter_criteria.h"
#include "../../model/game_library.h"
#include "../../model/launch_command.h"
#include "../../model/locations.h"
#include "../../model/metadata.h"
#include "../../model/metadata_form.h"

static GameLibrary testPool;
static bool testPoolReady;

static void makeGame( Game &game, const char *folder, const char *ini ) {
  if ( !testPoolReady ) {
    testPool.allocate();
    testPoolReady = true;
  }
  memset( &game, 0, sizeof( game ) );
  strcpy( game.folder, folder );
  strcpy( game.title, folder );
  char text[512];
  strcpy( text, ini );
  Metadata metadata;
  metadata.parse( text );
  metadata.storeRecord( game, testPool );
  if ( metadata.hasName() ) metadata.applyTitle( game );
}

TEST( metadataReadsTheValuesAndTheDescriptionLines ) {
  Game game;
  makeGame( game, "DOOM",
            "[GAME]\r\nNAME=  \"Doom\"\r\nGENRE=Action\tShooter\r\nDESCRIPTION=One\\nTwo\r\n"
            "DESCRIPTION=Back\\\\slash\r\nRUN=DOOM.EXE -nosound\r\n" );
  CHECK_STR( game.title, "Doom" );
  CHECK_STR( game.field( FieldGenre ), "Action Shooter" );
  CHECK_STR( game.field( FieldDesc ), "One\nTwo\nBack\\slash" );
  CHECK_STR( game.field( FieldRun ), "DOOM.EXE -nosound" );
  CHECK_STR( game.field( FieldArgs ), "" );
}

TEST( metadataWithoutValuesHasNoRecord ) {
  Game game;
  makeGame( game, "EMPTY", "; nothing\r\n" );
  CHECK( game.record == NULL );
  CHECK_STR( game.title, "EMPTY" );
}

TEST( titleIsTheFirstLineCut ) {
  Game game;
  memset( &game, 0, sizeof( game ) );
  strcpy( game.folder, "X" );
  const char *text = "  A very long title that does not fit into the fifty characters\r\nsecond";
  setTitle( game, text, strlen( text ) );
  CHECK_STR( game.title, "A very long title that does not fit into the fifty" );
  setTitle( game, "   \r\n", 5 );
  CHECK_STR( game.title, "X" );
}

TEST( filtersMatchDecadesRangesAndTheSearch ) {
  Game game;
  makeGame( game, "GOLF", "NAME=Arvizturo\r\nYEAR=1993\r\nPLAYERS=1-4\r\nGENRE=Sports\r\nDESCRIPTION=secretword\r\n" );
  FilterCriteria c;
  memset( &c, 0, sizeof( c ) );
  CHECK( c.matches( game ) );
  strcpy( c.year, "1990s" );
  strcpy( c.players, "3" );
  strcpy( c.genre, "SPORTS" );
  CHECK( c.matches( game ) );
  strcpy( c.players, "5" );
  CHECK( !c.matches( game ) );
  c.clearFilters();
  c.addToSearch( 'S' );
  c.addToSearch( 'e' );
  c.addToSearch( 'C' );
  CHECK( c.matches( game ) );
  c.clearSearch();
  c.favoritesOnly = 1;
  CHECK( !c.matches( game ) );
  CHECK( c.filtersActive() );
}

TEST( playerRangesParse ) {
  const char *s = "2+";
  unsigned a, b;
  CHECK( parseRange( &s, &a, &b ) );
  CHECK( a == 2 && b == 0xFFFF );
  s = "1 - 4";
  CHECK( parseRange( &s, &a, &b ) && a == 1 && b == 4 );
  s = "many";
  CHECK( !parseRange( &s, &a, &b ) );
}

TEST( libraryOrdersPutEmptyValuesLast ) {
  GameLibrary library;
  CHECK( library.allocate() );
  makeGame( library.at( library.add( "B" ) ), "B", "NAME=Bravo\r\nYEAR=1995\r\n" );
  makeGame( library.at( library.add( "A" ) ), "A", "NAME=Alpha\r\n" );
  makeGame( library.at( library.add( "C" ) ), "C", "NAME=Charlie\r\nYEAR=1990\r\n" );
  library.at( 1 ).plays = 3;
  library.sort( SortTitle );
  CHECK_STR( library.at( library.ordered( 0 ) ).folder, "A" );
  library.sort( SortYear );
  CHECK_STR( library.at( library.ordered( 0 ) ).folder, "C" );
  CHECK_STR( library.at( library.ordered( 2 ) ).folder, "A" );
  library.sort( SortPlays );
  CHECK_STR( library.at( library.ordered( 0 ) ).folder, "A" );
  CHECK_INT( library.find( "c" ), 2 );
  CHECK_STR( sortOrderName( SortPlays ), "most played" );
}

TEST( playedTextShowsTheDate ) {
  Game game;
  memset( &game, 0, sizeof( game ) );
  char text[32];
  game.playedText( text );
  CHECK_STR( text, "never" );
  game.plays = 5;
  game.last = ( ( 2026 - 1980 ) << 9 ) | ( 10 << 5 ) | 9;
  game.playedText( text );
  CHECK_STR( text, "5x, last 2026-10-09" );
}

static Locations testLocations() {
  Locations where;
  memset( &where, 0, sizeof( where ) );
  strcpy( where.progDir, "C:\\GMENU\\" );
  strcpy( where.startDir, "WORK" );
  where.homeDrive = 'C';
  where.gameDrive = 'C';
  where.setGameDir( "d:\\games\\" );
  return where;
}

TEST( locationsTakeTheGameDirectory ) {
  Locations where = testLocations();
  CHECK_STR( where.gameDir, "D:\\GAMES" );
  CHECK_INT( where.gameDrive, 'D' );
  CHECK_STR( where.gamePath(), "\\GAMES" );
  where.setProgDir( "GMENUPRG.EXE" );
  CHECK_STR( where.progDir, "\\GMENU\\" );
}

TEST( runScriptGoesToTheFolderAndBack ) {
  Locations where = testLocations();
  Game game;
  makeGame( game, "DOOM", "RUN=SETUP.BAT x\r\nARGS=-fast\r\n" );
  char command[CommandSize], script[ScriptSize];
  runCommand( game, command );
  CHECK_STR( command, "SETUP.BAT x -fast" );
  unsigned len = runScript( where, game, command, script );
  script[len] = 0;
  CHECK_STR( script, "@echo off\r\nD:\r\ncd \\GAMES\\DOOM\r\ncall SETUP.BAT x -fast\r\nC:\r\ncd \\WORK\r\n" );
  len = editScript( where, game, "C:\\GMENU\\GMEDIT.EXE", script );
  script[len] = 0;
  CHECK_STR( script, "@echo off\r\nC:\\GMENU\\GMEDIT.EXE /B D:\\GAMES\\DOOM\r\n" );
}

TEST( fileCommandsKeepTheirArguments ) {
  Game game;
  makeGame( game, "DOOM", "RUN=DOOM.EXE -a\r\nSETUP=SETUP.EXE -s\r\nARGS=-x\r\n" );
  char command[CommandSize];
  fileCommand( game, "doom.exe", command );
  CHECK_STR( command, "DOOM.EXE -a -x" );
  fileCommand( game, "SETUP.EXE", command );
  CHECK_STR( command, "SETUP.EXE -s" );
  fileCommand( game, "OTHER.BAT", command );
  CHECK_STR( command, "OTHER.BAT" );
  CHECK( commandRuns( "DOOM.EXE -a", "doom.exe" ) );
  CHECK( !commandRuns( "DOOM.EXE", "DOOM" ) );
}

TEST( formChangesOnlyTheChangedLines ) {
  static MetadataForm form;
  const char *raw =
      "\xEF\xBB\xBF[GAME]\r\n; mine\r\nNAME=Doom\r\nGENRE=Action\r\nDESCRIPTION=One\r\nDESCRIPTION=Two\r\nX=keep\n";
  form.load( raw );
  form.keepAsOriginal();
  CHECK_STR( form.value( FormDesc ), "One\\nTwo" );
  CHECK( !form.isChanged() );
  strcpy( form.value( FormGenre ), "Sh\x81ter" );
  strcpy( form.value( FormDesc ), "Only" );
  form.value( FormName )[0] = 0;
  strcpy( form.value( FormYear ), "1993" );
  CHECK( form.isChanged() );
  static char out[MetadataForm::OutSize];
  out[form.write( out )] = 0;
  CHECK_STR( out,
             "\xEF\xBB\xBF[GAME]\r\n; mine\r\nGENRE=Sh\xC3\xBCter\r\nDESCRIPTION=Only\r\nX=keep\r\nYEAR=1993\r\n" );
}

TEST( newFormStartsFromTheFolder ) {
  static MetadataForm form;
  ExecutableList exes;
  exes.clear();
  exes.add( "SETUP.EXE" );
  exes.add( "DOOM.EXE" );
  exes.add( "INSTALL.BAT" );
  form.keepAsOriginal();
  form.prefill( NULL, -1, "C:\\COLLECT\\DOOM", exes );
  CHECK_STR( form.value( FormName ), "DOOM" );
  CHECK_STR( form.value( FormRun ), "DOOM.EXE" );
  CHECK_STR( form.value( FormSetup ), "INSTALL.BAT" );
  static char out[MetadataForm::OutSize];
  out[form.write( out )] = 0;
  CHECK_STR( out, "[GAME]\r\nNAME=DOOM\r\nRUN=DOOM.EXE\r\nSETUP=INSTALL.BAT\r\n" );
}
