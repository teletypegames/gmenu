#include "frame.h"
#include "scan_view.h"
#include "../lib/ui/canvas.h"

enum { Top = 10, Left = 18, Right = 61 };

ScanView::ScanView( Frame &frame ) : frame_( frame ) {}

void ScanView::report( const char *text ) {
  Canvas &canvas = frame_.canvas();
  frame_.beginPlain();
  canvas.box( Top, Left, Top + 2, Right, 0x13 );
  int pos = canvas.textAt( Top + 1, Left + 2, text, Right - Left - 3, 0x1F );
  canvas.fillTo( pos, Canvas::at( Top + 1, Right ), 0x1F );
  frame_.end();
}
