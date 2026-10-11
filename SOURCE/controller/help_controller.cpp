#include "help_controller.h"
#include "menu_input.h"
#include "../lib/ui/keys.h"
#include "../view/help_view.h"
#include "../view/layout.h"

HelpController::HelpController( HelpView &view, MenuInput &input ) : view_( view ), input_( input ) {}

bool HelpController::scrollOf( unsigned key, int &rows ) {
  static const struct {
    unsigned char scan;
    signed char rows;
  } keys[] = { { ScanUp, -1 },         { ScanDown, 1 },    { ScanPgUp, -HelpRows },
               { ScanPgDn, HelpRows }, { ScanHome, -128 }, { ScanEnd, 127 } };
  if ( key == InputWheelUp ) rows = -MenuInput::WheelStep;
  else if ( key == InputWheelDown ) rows = MenuInput::WheelStep;
  else {
    if ( !isExtendedKey( key ) ) return false;
    unsigned i;
    for ( i = 0; i < sizeof( keys ) / sizeof( keys[0] ) && keys[i].scan != keyScan( key ); i++ );
    if ( i == sizeof( keys ) / sizeof( keys[0] ) ) return false;
    rows = keys[i].rows;
  }
  return true;
}

void HelpController::run() {
  int top = 0;
  int max = HelpView::lineCount() - HelpRows;
  if ( max < 0 ) max = 0;
  for ( ;; ) {
    view_.draw( top );
    int rows;
    if ( !scrollOf( input_.next(), rows ) ) return;
    top += rows;
    if ( top > max ) top = max;
    if ( top < 0 ) top = 0;
  }
}
