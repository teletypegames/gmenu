#include "list_runner.h"
#include "menu_input.h"
#include "../lib/core/str.h"
#include "../lib/ui/keys.h"

enum { HeaderRow = 0, FooterRow = 24 };

ListRunner::ListRunner( MenuInput &input ) : input_( input ), page_( 0 ), behavior_( 0 ), cursor_( 0 ) {}

ListResult ListRunner::run( ListPage &page, const ListBehavior &behavior, ListCursor &cursor, unsigned &key ) {
  page_ = &page;
  behavior_ = &behavior;
  cursor_ = &cursor;
  cursor.clamp();
  for ( ;; ) {
    page.draw( cursor );
    Step step;
    do step = handle( key );
    while ( step == StepWait );
    if ( step == StepSelected ) return ListSelected;
    if ( step == StepEsc ) return ListEsc;
    if ( step == StepKey ) return ListKey;
  }
}

ListRunner::Step ListRunner::moveTo( int index ) {
  if ( cursor_->isEmpty() ) return StepWait;
  cursor_->select( index );
  return StepRedraw;
}

ListRunner::Step ListRunner::selected() {
  return cursor_->isEmpty() ? StepWait : StepSelected;
}

static int clampIndex( int i, int count ) {
  if ( i >= count ) i = count - 1;
  if ( i < 0 ) i = 0;
  return i;
}

ListRunner::Step ListRunner::handle( unsigned &key ) {
  key = input_.next();
  if ( key == InputClick ) return handleClick( key );
  if ( key == InputWheelUp || key == InputWheelDown ) return handleWheel( key );
  return handleKey( key );
}

ListRunner::Step ListRunner::handleWheel( unsigned key ) {
  if ( behavior_->paneEnd && input_.col() < behavior_->paneEnd ) return StepKey;
  int step = key == InputWheelUp ? -MenuInput::WheelStep : MenuInput::WheelStep;
  int sel = cursor_->sel() + step;
  if ( step < 0 && sel < 0 ) sel = 0;
  if ( step > 0 && sel >= cursor_->count() ) sel = cursor_->count() - 1;
  return moveTo( sel );
}

ListRunner::Step ListRunner::handleKey( unsigned key ) {
  unsigned char ch = keyAscii( key ), scan = keyScan( key );
  int sel = cursor_->sel(), count = cursor_->count(), rows = cursor_->rows();
  if ( ch == KeyEsc ) return StepEsc;
  if ( ch == KeyEnter ) return selected();
  if ( isExtendedKey( key ) ) {
    if ( behavior_->shiftPagesToCaller && ( scan == ScanPgUp || scan == ScanPgDn ) && input_.shiftDown() )
      return StepKey;
    switch ( scan ) {
      case ScanUp: return moveTo( sel ? sel - 1 : count - 1 );
      case ScanDown: return moveTo( sel + 1 >= count ? 0 : sel + 1 );
      case ScanPgUp: return moveTo( sel - rows < 0 ? 0 : sel - rows );
      case ScanPgDn: return moveTo( sel + rows >= count ? count - 1 : sel + rows );
      case ScanHome: return moveTo( 0 );
      case ScanEnd: return moveTo( count - 1 );
      case ScanLeft: return StepEsc;
      case ScanRight: return selected();
    }
    return StepKey;
  }
  if ( ch <= ' ' || behavior_->typingSearches ) return StepKey;
  jumpTo( ch );
  cursor_->scrollToSelection();
  return StepRedraw;
}

void ListRunner::jumpTo( char c ) {
  c = upcase( c );
  int i = cursor_->sel();
  for ( int n = cursor_->count(); n; n-- ) {
    if ( ++i >= cursor_->count() ) i = 0;
    if ( upcase( page_->label( i )[0] ) == c ) {
      cursor_->place( i, cursor_->top() );
      return;
    }
  }
}

ListRunner::Step ListRunner::handleClick( unsigned &key ) {
  int sel = cursor_->sel(), rows = cursor_->rows(), count = cursor_->count();
  if ( input_.row() == HeaderRow ) return moveTo( clampIndex( sel - rows, count ) );
  if ( input_.row() >= FooterRow ) return moveTo( sel + rows >= count ? count - 1 : sel + rows );
  int r = input_.row() - behavior_->firstRow;
  if ( r < 0 || r >= rows ) return StepWait;
  if ( behavior_->paneEnd && input_.col() < behavior_->paneEnd ) return StepWait;
  r += cursor_->top();
  if ( r >= count ) return StepWait;
  if ( !behavior_->gameList ) {
    cursor_->place( r, cursor_->top() );
    page_->draw( *cursor_ );
    return StepSelected;
  }
  if ( input_.doubleClick() ) {
    cursor_->place( r, cursor_->top() );
    key = InputDoubleClick;
    return StepKey;
  }
  if ( r == sel ) {
    if ( !input_.waitSecondClick() ) return selected();
    key = InputDoubleClick;
    return StepKey;
  }
  cursor_->select( r );
  return StepRedraw;
}
