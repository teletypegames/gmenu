#include <string.h>

#include "picker_controller.h"
#include "../lib/platform/speaker.h"
#include "../model/session.h"
#include "../model/visible_games.h"
#include "../service/library_service.h"
#include "../service/picker_service.h"
#include "../view/layout.h"

static const ListBehavior Picker = { ListFirstRow, false, false, 0, false };

PickerController::PickerController( PickerService &picker, LibraryService &library, Session &session,
                                    VisibleGames &visible, PickerView &view, ListRunner &runner, Speaker &speaker )
  : picker_( picker ), library_( library ), session_( session ), visible_( visible ), view_( view ), runner_( runner ),
    speaker_( speaker ) {
  cursor_.setRows( PickRows );
}

void PickerController::draw( const ListCursor &cursor ) {
  view_.draw( cursor, picks_, box_ );
}

const char *PickerController::label( int i ) {
  return picks_.items[i];
}

bool PickerController::run( int sel ) {
  unsigned key;
  ListResult result;
  cursor_.setCount( picks_.count );
  cursor_.place( sel, 0 );
  while ( ( result = runner_.run( *this, Picker, cursor_, key ) ) == ListKey );
  return result == ListSelected;
}

void PickerController::pickFilter( int field, const char *title ) {
  char *filter = session_.criteria.filterOf( field );
  int sel = picker_.filterValues( field, filter, picks_ );
  box_.left = 0x18;
  box_.right = 0x37;
  box_.title = title;
  if ( !run( sel ) ) return;
  if ( cursor_.sel() == 0 ) filter[0] = 0;
  else strcpy( filter, picks_.items[cursor_.sel()] );
  library_.refilter();
}

bool PickerController::pickRecent( int &game ) {
  if ( picker_.recentGames( session_.recent, picks_ ) == 0 ) {
    speaker_.beep();
    return false;
  }
  box_.left = 0x0A;
  box_.right = 0x45;
  box_.title = "Recently played";
  if ( !run( 0 ) ) return false;
  game = picks_.games[cursor_.sel()];
  visible_.select( game );
  return true;
}
