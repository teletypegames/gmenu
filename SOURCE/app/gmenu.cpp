#include <stddef.h>

#include "gmenu.h"
#include "machine.h"
#include "../controller/detail_controller.h"
#include "../controller/game_list_controller.h"
#include "../controller/help_controller.h"
#include "../controller/menu_input.h"
#include "../controller/picker_controller.h"
#include "../lib/core/str.h"
#include "../lib/platform/mouse.h"
#include "../lib/platform/screen.h"
#include "../lib/platform/text_output.h"
#include "../lib/ui/canvas.h"
#include "../model/file_list.h"
#include "../model/game_library.h"
#include "../model/locations.h"
#include "../model/session.h"
#include "../model/visible_games.h"
#include "../repository/files/bat_launcher_repository.h"
#include "../repository/files/fav_file_repository.h"
#include "../repository/files/folder_files_repository.h"
#include "../repository/files/folder_game_repository.h"
#include "../repository/files/sel_file_repository.h"
#include "../repository/files/sta_file_repository.h"
#include "../service/launch_service.h"
#include "../service/library_service.h"
#include "../service/picker_service.h"
#include "../view/detail_view.h"
#include "../view/frame.h"
#include "../view/game_list_view.h"
#include "../view/help_view.h"
#include "../view/layout.h"
#include "../view/picker_view.h"
#include "../view/scan_view.h"

/* The menu put together: the members are built in this order. */
struct Gmenu {
  Gmenu( Machine &m, const Locations &where );

  GameLibrary library;
  Session session;
  VisibleGames visible;
  FileList files;
  FolderGameRepository gameFolders;
  FavFileRepository favorites;
  StaFileRepository stats;
  SelFileRepository sessions;
  BatLauncherRepository launchers;
  FolderFilesRepository folderFiles;
  LibraryService games;
  LaunchService launcher;
  PickerService picker;
  Canvas canvas;
  Frame frame;
  GameListView listView;
  DetailView detailView;
  PickerView pickerView;
  HelpView helpView;
  ScanView scanView;
  MenuInput input;
  ListRunner runner;
  HelpController help;
  DetailController detail;
  PickerController pickers;
  GameListController menu;
};

Gmenu::Gmenu( Machine &m, const Locations &where )
  : session(), visible(), gameFolders( m.fs, where ), favorites( m.fs, where ), stats( m.fs, where ),
    sessions( m.fs, where ), launchers( m.fs, where ), folderFiles( m.fs, where ),
    games( library, session, visible, favorites, sessions, m.clock ),
    launcher( library, session, where, launchers, stats, folderFiles, m.clock ), picker( library ), canvas( m.screen ),
    frame( canvas, m.mouse ), listView( frame, library, session, visible, where ), detailView( frame, where ),
    pickerView( frame ), helpView( frame ), scanView( frame ), input( m.keyboard, m.mouse, m.idle, m.clock ),
    runner( input ), help( helpView, input ),
    detail( library, session, games, launcher, detailView, runner, help, m.speaker ),
    pickers( picker, games, session, visible, pickerView, runner, m.speaker ),
    menu( library, session, visible, games, launcher, listView, runner, detail, pickers, help, m.speaker, files ) {
  visible.rows = GameRows;
}

struct Options {
  bool fromBatch; /* /B: only GMENU.BAT starts the game the menu chose */
  bool allFiles;  /* /A */
};

static Options parseArgs( Locations &where, int argc, char *argv[] ) {
  Options options = { false, false };
  const char *dir = NULL;
  for ( int i = 1; i < argc; i++ ) {
    const char *a = argv[i];
    if ( a[0] != '/' ) dir = a;
    else if ( upcase( a[1] ) == 'B' ) options.fromBatch = true;
    else if ( upcase( a[1] ) == 'A' ) options.allFiles = true;
  }
  if ( dir ) where.setGameDir( dir );
  return options;
}

static void restoreScreen( Machine &m ) {
  m.mouse.reset();
  m.screen.end();
}

/* The games with their favorites and statistics, and the session; false when there are
   none. rescan: the folders are scanned, not taken from GMENU.IDX. */
static bool loadGames( Gmenu &app, bool rescan ) {
  if ( rescan ) app.gameFolders.scan( app.library, app.scanView );
  else app.gameFolders.load( app.library, app.scanView );
  if ( app.library.count() == 0 ) return false;
  app.favorites.load( app.library );
  app.stats.load( app.library );
  app.games.restore();
  /* the selected game may have just been edited (GMEDIT) */
  int g = app.visible.current();
  if ( !rescan && g >= 0 && app.gameFolders.recheck( app.library, g ) ) app.games.reorder();
  return true;
}

static int runMenu( Machine &m, Gmenu &app, const Locations &where, const Options &options ) {
  m.screen.begin();
  m.screen.hideCursor();
  m.mouse.detect();
  bool rescan = false;
  Outcome outcome;
  for ( ;; ) {
    if ( !loadGames( app, rescan ) ) {
      restoreScreen( m );
      m.out.write( "No game folders found in: " );
      m.out.write( where.gameDir );
      m.out.write( "\n" );
      return 2;
    }
    if ( options.allFiles ) app.session.showAll = 1;
    m.mouse.show();
    outcome = app.menu.run();
    app.games.saveSession();
    if ( outcome != OutcomeRescan ) break;
    rescan = true;
  }
  restoreScreen( m );
  return outcome == OutcomeLaunch ? 0 : 1;
}

int runGmenu( Machine &m, Locations &where, int argc, char *argv[] ) {
  Options options = parseArgs( where, argc, argv );
  if ( !options.fromBatch ) {
    m.out.write( "Start the menu with GMENU.BAT (type GMENU).\n" );
    return 2;
  }
  Gmenu *app = new Gmenu( m, where );
  if ( app == NULL || !app->library.allocate() ) {
    m.out.write( "Not enough memory.\n" );
    delete app;
    return 2;
  }
  int code = runMenu( m, *app, where, options );
  delete app;
  return code;
}
