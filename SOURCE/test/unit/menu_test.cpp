#include <string.h>

#include "../../app/gmenu.h"
#include "../../app/gmedit.h"
#include "../../app/machine.h"
#include "../../lib/platform/idle.h"
#include "../../lib/platform/mouse.h"
#include "../../lib/platform/speaker.h"
#include "../../lib/platform/text_output.h"
#include "../../lib/testing/host/fake_clock.h"
#include "../../lib/testing/host/fake_file_system.h"
#include "../../lib/testing/host/memory_screen.h"
#include "../../lib/testing/host/minitest.h"
#include "../../lib/testing/scripted_keyboard.h"
#include "../../model/game_library.h"
#include "../../model/locations.h"
#include "../../model/session.h"
#include "../../lib/core/progress.h"
#include "../../lib/core/str.h"
#include "../../repository/files/folder_game_repository.h"
#include "../../repository/files/index_file.h"
#include "../../repository/files/sel_file_repository.h"

class NoMouse : public Mouse {
public:
  bool detect() { return false; }
  bool present() const { return false; }
  bool hasWheel() const { return false; }
  void reset() {}
  void show() {}
  void hide() {}
  void poll( MouseState & ) {}
  unsigned leftPresses() { return 0; }
};

class NoIdle : public Idle {
public:
  void idle() {}
};

class CountingSpeaker : public Speaker {
public:
  CountingSpeaker() : beeps( 0 ) {}
  void beep() { beeps++; }
  int beeps;
};

class CapturedOutput : public TextOutput {
public:
  CapturedOutput() { text[0] = 0; }
  void write( const char *s ) { strcat( text, s ); }
  char text[400];
};

/* Ends the script like the test builds do: the screen is kept (its first row) when the
   keys run out, then Esc follows until the program ends. */
class StopAtTheEnd : public ScriptEnd {
public:
  StopAtTheEnd() : screen( NULL ) { header[0] = 0; }
  unsigned scriptEnded() {
    if ( screen && !header[0] ) strcpy( header, screen->text( 0 ) );
    return 0x011B;
  }
  MemoryScreen *screen;
  char header[81];
};

static Locations where() {
  Locations w;
  memset( &w, 0, sizeof( w ) );
  strcpy( w.gameDir, "C:\\COLLECT" );
  strcpy( w.progDir, "C:\\GMENU\\" );
  w.homeDrive = w.gameDrive = 'C';
  return w;
}

static void collection( FakeFileSystem &fs ) {
  fs.makeFolder( "C:\\GMENU" );
  fs.add( "C:\\COLLECT\\ALPHA\\METADATA.INI", "NAME=Alpha\r\nGENRE=Action\r\nYEAR=1991\r\nRUN=ALPHA.EXE\r\n" );
  fs.add( "C:\\COLLECT\\ALPHA\\ALPHA.EXE", "" );
  fs.add( "C:\\COLLECT\\BRAVO\\NAME.TXT", "Bravo Two\r\n" );
  fs.add( "C:\\COLLECT\\BRAVO\\B.EXE", "" );
  fs.add( "C:\\COLLECT\\CHARLIE\\C1.EXE", "" );
  fs.add( "C:\\COLLECT\\CHARLIE\\C2.BAT", "" );
  fs.add( "C:\\COLLECT\\README.TXT", "not a game" );
}

class NoProgress : public Progress {
public:
  NoProgress() : reports( 0 ) { last[0] = 0; }
  void report( const char *text ) {
    reports++;
    strcpy( last, text );
  }
  int reports;
  char last[80];
};

TEST( gamesComeFromTheFilesThenFromTheIndex ) {
  FakeFileSystem fs;
  collection( fs );
  Locations w = where();
  NoProgress progress;
  static GameLibrary first, second;
  CHECK( first.allocate() && second.allocate() );
  FolderGameRepository( fs, w ).load( first, progress );
  CHECK_INT( first.count(), 3 );
  CHECK_STR( first.at( 1 ).title, "Bravo Two" );
  CHECK( first.at( 1 ).flags & GameNoMetadata );
  CHECK( fs.has( "C:\\GMENU\\GMENU.IDX" ) );
  CHECK( progress.reports > 0 );

  /* the next start takes the games from GMENU.IDX, without looking at the folders */
  progress.reports = 0;
  fs.add( "C:\\COLLECT\\ALPHA\\METADATA.INI", "NAME=Changed\r\n", 0x5B4A, 0x6001 );
  fs.add( "C:\\COLLECT\\DELTA\\D.EXE", "" );
  FolderGameRepository( fs, w ).load( second, progress );
  CHECK_INT( second.count(), 3 );
  CHECK_STR( second.at( 0 ).title, "Alpha" );
  CHECK_STR( second.at( 0 ).field( FieldYear ), "1991" );
  CHECK( second.at( 1 ).flags & GameNoMetadata );
  CHECK( second.at( 1 ).flags & GameTitleNotFromMetadata );
  CHECK_INT( progress.reports, 0 );

  /* a scan (Ctrl+F) finds the new folder and the changed file */
  FolderGameRepository( fs, w ).scan( second, progress );
  CHECK_INT( second.count(), 4 );
  CHECK_STR( second.at( 0 ).title, "Changed" );
  CHECK_STR( second.at( 3 ).folder, "DELTA" );
}

TEST( recheckReadsTheChangedGame ) {
  FakeFileSystem fs;
  collection( fs );
  Locations w = where();
  NoProgress progress;
  static GameLibrary library, again;
  CHECK( library.allocate() && again.allocate() );
  FolderGameRepository games( fs, w );
  games.load( library, progress );
  library.at( 0 ).flags |= GameFavorite;
  CHECK( !games.recheck( library, 0 ) );
  fs.add( "C:\\COLLECT\\ALPHA\\METADATA.INI", "NAME=Edited\r\nYEAR=1999\r\n", 0x5B4A, 0x6002 );
  CHECK( games.recheck( library, 0 ) );
  CHECK_STR( library.at( 0 ).title, "Edited" );
  CHECK_STR( library.at( 0 ).field( FieldYear ), "1999" );
  CHECK( library.at( 0 ).isFavorite() );
  /* and GMENU.IDX has it */
  FolderGameRepository( fs, w ).load( again, progress );
  CHECK_STR( again.at( 0 ).title, "Edited" );
  fs.add( "C:\\COLLECT\\BRAVO\\NAME.TXT", "Bravo Three\r\n", 0x5B4A, 0x6003 );
  CHECK( games.recheck( library, 1 ) );
  CHECK_STR( library.at( 1 ).title, "Bravo Three" );
}

TEST( indexHoldsMoreThan600Games ) {
  FakeFileSystem fs;
  fs.makeFolder( "C:\\GMENU" );
  Locations w = where();
  static GameLibrary many, loaded;
  CHECK( many.allocate() && loaded.allocate() );
  for ( int i = 0; i < 2000; i++ ) {
    char folder[FolderSize] = "G";
    numStr( folder + 1, i );
    int g = many.add( folder );
    CHECK( g == i );
    strcpy( many.at( g ).title, folder );
    char *record = many.allocRecord( 7 + 200 );
    CHECK( record != NULL );
    memset( record, 0, 7 + 200 );
    memset( record + 3, 'd', 200 );
    many.at( g ).record = record;
  }
  IndexFile index( fs, "C:\\GMENU\\GMENU.IDX", w.gameDir );
  index.save( many );
  CHECK( index.loadAll( loaded ) );
  CHECK_INT( loaded.count(), 2000 );
  CHECK_STR( loaded.at( 1999 ).title, "G1999" );
  CHECK_INT( (int)strlen( loaded.at( 1999 ).field( FieldDesc ) ), 200 );
  CHECK_INT( loaded.find( "G1234" ), 1234 );
  loaded.clear();
  CHECK_INT( loaded.count(), 0 );
}

TEST( sessionFileRoundTrips ) {
  FakeFileSystem fs;
  fs.makeFolder( "C:\\GMENU" );
  Locations w = where();
  SelFileRepository files( fs, w );
  static Session saved, loaded;
  memset( &saved, 0, sizeof( saved ) );
  strcpy( saved.selected, "BRAVO" );
  saved.criteria.addToSearch( 'b' );
  strcpy( saved.criteria.year, "1990s" );
  saved.sortOrder = SortGenre;
  saved.recent.add( "ALPHA" );
  files.save( saved );
  CHECK_INT( fs.size( "C:\\GMENU\\GMENU.SEL" ), 230 );
  CHECK( files.load( loaded ) );
  CHECK_STR( loaded.selected, "BRAVO" );
  CHECK_STR( loaded.criteria.search, "b" );
  CHECK_STR( loaded.criteria.year, "1990s" );
  CHECK_INT( loaded.sortOrder, SortGenre );
  CHECK_STR( loaded.recent.folders[0], "ALPHA" );
}

struct Menu {
  FakeFileSystem fs;
  MemoryScreen screen;
  NoMouse mouse;
  FakeClock clock;
  NoIdle idle;
  CountingSpeaker speaker;
  CapturedOutput out;
  StopAtTheEnd end;

  Menu() {
    collection( fs );
    end.screen = &screen;
  }

  int run( const char *keys, const char *args = "/B" ) {
    ScriptedKeyboard keyboard( keys, end );
    Machine m = { fs, screen, keyboard, mouse, clock, idle, speaker, out };
    Locations w = where();
    char arg[16];
    strcpy( arg, args );
    char *argv[] = { (char *)"GMENUPRG.EXE", arg, NULL };
    return runGmenu( m, w, 2, argv );
  }
};

TEST( menuNeedsTheBatchFile ) {
  Menu menu;
  CHECK_INT( menu.run( "", "/X" ), 2 );
  CHECK_STR( menu.out.text, "Start the menu with GMENU.BAT (type GMENU).\n" );
}

TEST( searchThenPlayWritesTheRunner ) {
  Menu menu;
  CHECK_INT( menu.run( "62 72 1C0A" ), 0 );
  CHECK_STR( menu.fs.data( "C:\\GMENU\\GMENURUN.BAT" ),
             "@echo off\r\nC:\r\ncd \\COLLECT\\BRAVO\r\nB.EXE\r\nC:\r\ncd \\\r\n" );
  CHECK_INT( menu.fs.size( "C:\\GMENU\\GMENU.STA" ), 17 );
}

TEST( gamePageListsTheExecutables ) {
  Menu menu;
  CHECK_INT( menu.run( "5000 5000 1C0D" ), 1 );
  CHECK_STR( menu.end.header, " CHARLIE  (CHARLIE)                                                 1/2" );
}

TEST( setupWithoutSetupBeeps ) {
  Menu menu;
  CHECK_INT( menu.run( "1F13" ), 1 );
  CHECK_INT( menu.speaker.beeps, 1 );
}

TEST( editorWritesTheMetadata ) {
  FakeFileSystem fs;
  collection( fs );
  MemoryScreen screen;
  CountingSpeaker speaker;
  CapturedOutput out;
  StopAtTheEnd end;
  ScriptedKeyboard keyboard( "0F09 1E41 3C00", end );
  EditorMachine m = { fs, screen, keyboard, speaker, out };
  char folder[] = "c:\\collect\\charlie\\";
  char *argv[] = { (char *)"GMEDIT.EXE", folder, NULL };
  CHECK_INT( runGmedit( m, 2, argv ), 0 );
  CHECK_STR( fs.data( "C:\\COLLECT\\CHARLIE\\METADATA.INI" ), "[GAME]\r\nNAME=CHARLIE\r\nGENRE=A\r\n" );
}

TEST( ctrlFScansTheFoldersAgain ) {
  Menu menu;
  CHECK_INT( menu.run( "" ), 1 );
  menu.fs.add( "C:\\COLLECT\\DELTA\\D.EXE", "" );
  menu.end.header[0] = 0;
  CHECK_INT( menu.run( "" ), 1 );
  CHECK( strstr( menu.end.header, "/3" ) != NULL );
  menu.end.header[0] = 0;
  CHECK_INT( menu.run( "2106" ), 1 );
  CHECK( strstr( menu.end.header, "/4" ) != NULL );
  /* the next start has it from GMENU.IDX */
  menu.end.header[0] = 0;
  CHECK_INT( menu.run( "" ), 1 );
  CHECK( strstr( menu.end.header, "/4" ) != NULL );
}

TEST( editedGameIsReadAgainAtTheStart ) {
  Menu menu;
  CHECK_INT( menu.run( "5000 5000" ), 1 );
  menu.fs.add( "C:\\COLLECT\\CHARLIE\\METADATA.INI", "NAME=Aardvark\r\n", 0x5B4A, 0x6005 );
  menu.end.header[0] = 0;
  CHECK_INT( menu.run( "1C0D" ), 1 );
  CHECK( strstr( menu.end.header, "Aardvark" ) != NULL );
}
