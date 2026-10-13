#include "detail_controller.h"
#include "game_list_controller.h"
#include "help_controller.h"
#include "menu_input.h"
#include "picker_controller.h"
#include "../lib/platform/speaker.h"
#include "../lib/ui/keys.h"
#include "../model/game_library.h"
#include "../model/session.h"
#include "../model/visible_games.h"
#include "../service/launch_service.h"
#include "../service/library_service.h"
#include "../view/game_list_view.h"
#include "../view/layout.h"

enum { KeyCtrlF = 0x06 };

static const ListBehavior Games = { ListFirstRow, true, true, 0, false };

GameListController::GameListController( GameLibrary &library, Session &session, VisibleGames &visible,
                                        LibraryService &games, LaunchService &launcher, GameListView &view,
                                        ListRunner &runner, DetailController &detail, PickerController &picker,
                                        HelpController &help, Speaker &speaker, FileList &files )
  : library_( library ), session_( session ), visible_( visible ), games_( games ), launcher_( launcher ),
    view_( view ), runner_( runner ), detail_( detail ), picker_( picker ), help_( help ), speaker_( speaker ),
    files_( files ) {
  cursor_.setRows( GameRows );
}

void GameListController::draw( const ListCursor &cursor ) {
  view_.draw( cursor );
}

const char *GameListController::label( int i ) {
  return library_.at( visible_.games[i] ).folder;
}

Outcome GameListController::run() {
  for ( ;; ) {
    unsigned key;
    cursor_.setCount( visible_.count );
    cursor_.place( visible_.sel, visible_.top );
    ListResult result = runner_.run( *this, Games, cursor_, key );
    visible_.sel = cursor_.sel();
    visible_.top = cursor_.top();
    Outcome outcome;
    if ( result == ListSelected ) outcome = detail_.run( visible_.games[visible_.sel] );
    else if ( result == ListKey ) outcome = handleKey( key );
    else outcome = handleEsc();
    if ( outcome != OutcomeBack ) return outcome;
  }
}

Outcome GameListController::handleEsc() {
  if ( session_.criteria.searchLen ) games_.clearSearch();
  else if ( session_.criteria.filtersActive() ) games_.clearFilters();
  else return OutcomeQuit;
  return OutcomeBack;
}

Outcome GameListController::play( int game ) {
  if ( launcher_.play( game, files_ ) ) return OutcomeLaunch;
  return detail_.run( game );
}

Outcome GameListController::setup( int game ) {
  if ( launcher_.setup( game ) ) return OutcomeLaunch;
  speaker_.beep();
  return OutcomeBack;
}

Outcome GameListController::handleKey( unsigned key ) {
  int game = visible_.current();
  if ( key == InputDoubleClick || keyAscii( key ) == KeyCtrlEnter ) return game >= 0 ? play( game ) : OutcomeBack;
  if ( !isExtendedKey( key ) ) return handleText( keyAscii( key ), game );
  return handleFunction( keyScan( key ), game );
}

Outcome GameListController::handleText( unsigned char ch, int game ) {
  if ( ch == KeyCtrlS ) return game >= 0 ? setup( game ) : OutcomeBack;
  if ( ch == KeyCtrlE ) return game >= 0 ? detail_.editMetadata( game ) : OutcomeBack;
  if ( ch == KeyCtrlF ) return OutcomeRescan;
  if ( ch == KeyBackspace ) games_.backspace();
  else if ( ch >= ' ' ) games_.addToSearch( ch );
  return OutcomeBack;
}

Outcome GameListController::handleFunction( unsigned char scan, int game ) {
  switch ( scan ) {
    case ScanF1: help_.run(); break;
    case ScanF2: picker_.pickFilter( FieldGenre, "Filter by genre" ); break;
    case ScanF3: picker_.pickFilter( FieldYear, "Filter by year" ); break;
    case ScanF4: picker_.pickFilter( FieldPlayers, "Filter by players" ); break;
    case ScanF5: games_.clearFilters(); break;
    case ScanF6:
      if ( game < 0 ) break;
      games_.toggleFavorite( game );
      {
        int sel = visible_.sel;
        games_.applyFilter();
        visible_.sel = sel;
      }
      break;
    case ScanF7: games_.toggleFavoritesOnly(); break;
    case ScanShiftF7: games_.toggleNoMetadataOnly(); break;
    case ScanF8:
      if ( picker_.pickRecent( game ) ) return detail_.run( game );
      break;
    case ScanF9: games_.nextSortOrder(); break;
    case ScanF10: games_.selectRandom(); break;
  }
  return OutcomeBack;
}
