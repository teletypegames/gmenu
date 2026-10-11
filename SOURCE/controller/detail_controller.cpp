#include "detail_controller.h"
#include "help_controller.h"
#include "menu_input.h"
#include "../lib/core/str.h"
#include "../lib/platform/speaker.h"
#include "../lib/ui/keys.h"
#include "../model/game_library.h"
#include "../model/session.h"
#include "../service/launch_service.h"
#include "../service/library_service.h"
#include "../view/detail_view.h"
#include "../view/layout.h"

static const ListBehavior Files = { ListFirstRow, false, false, FileCol - 1, true };

DetailController::DetailController( GameLibrary &library, Session &session, LibraryService &games,
                                    LaunchService &launcher, DetailView &view, ListRunner &runner, HelpController &help,
                                    Speaker &speaker )
  : library_( library ), session_( session ), games_( games ), launcher_( launcher ), view_( view ), runner_( runner ),
    help_( help ), speaker_( speaker ), game_( 0 ), scroll_( 0 ), lines_( 0 ) {
  cursor_.setRows( FileRows );
}

void DetailController::draw( const ListCursor &cursor ) {
  lines_ = view_.draw( cursor, library_.at( game_ ), files_, session_.showAll != 0, scroll_ );
}

const char *DetailController::label( int i ) {
  return files_.sorted[i];
}

void DetailController::rescan() {
  launcher_.listFiles( game_, session_.showAll != 0, files_ );
  cursor_.setCount( files_.count );
  cursor_.place( launcher_.runFileIndex( game_, files_ ), 0 );
}

Outcome DetailController::editMetadata( int game ) {
  if ( launcher_.editMetadata( game ) ) return OutcomeLaunch;
  speaker_.beep();
  return OutcomeBack;
}

bool DetailController::scrollOf( unsigned key, int &rows ) {
  if ( key == InputWheelUp ) rows = -MenuInput::WheelStep;
  else if ( key == InputWheelDown ) rows = MenuInput::WheelStep;
  else if ( !isExtendedKey( key ) ) return false;
  else if ( keyScan( key ) == ScanCtrlPgUp || keyScan( key ) == ScanPgUp ) rows = -( DescRows - 1 );
  else if ( keyScan( key ) == ScanCtrlPgDn || keyScan( key ) == ScanPgDn ) rows = DescRows - 1;
  else return false;
  return true;
}

void DetailController::scrollBy( int rows ) {
  int d = scroll_ + rows;
  if ( d > lines_ - DescRows ) d = lines_ - DescRows;
  if ( d < 0 ) d = 0;
  scroll_ = d;
}

Outcome DetailController::run( int game ) {
  game_ = game;
  scroll_ = 0;
  rescan();
  for ( ;; ) {
    unsigned key;
    ListResult result = runner_.run( *this, Files, cursor_, key );
    if ( result == ListEsc ) return OutcomeBack;
    if ( result == ListSelected ) {
      const char *file = files_.sorted[cursor_.sel()];
      if ( isExecutable( file ) ) {
        launcher_.runFile( game_, file );
        return OutcomeLaunch;
      }
      speaker_.beep();
      continue;
    }
    if ( keyScan( key ) == ScanTab ) {
      session_.showAll ^= 1;
      rescan();
      continue;
    }
    int rows;
    if ( keyAscii( key ) == KeyCtrlE ) {
      if ( editMetadata( game_ ) == OutcomeLaunch ) return OutcomeLaunch;
    }
    else if ( isExtendedKey( key ) && keyScan( key ) == ScanF6 ) {
      games_.toggleFavorite( game_ );
      games_.applyFilter();
    }
    else if ( isExtendedKey( key ) && keyScan( key ) == ScanF1 ) help_.run();
    else if ( scrollOf( key, rows ) ) scrollBy( rows );
  }
}
