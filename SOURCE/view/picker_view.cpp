#include "frame.h"
#include "layout.h"
#include "picker_view.h"
#include "../lib/ui/canvas.h"
#include "../lib/ui/list_cursor.h"
#include "../model/pick_list.h"

static const char Keys[] = "~\x18\x19~ Move  ~Enter~ Select  ~A-Z~ Jump  ~Esc~ Cancel";

PickerView::PickerView( Frame &frame ) : frame_( frame ) {}

void PickerView::draw( const ListCursor &cursor, const PickList &picks, const PickerBox &box ) {
  Canvas &canvas = frame_.canvas();
  frame_.begin( box.title, cursor );
  canvas.fillTo( Canvas::Cols, Canvas::Cols * 24, 0x17 );
  canvas.box( 1, box.left, 23, box.right, 0x13 );
  canvas.boxTitle( 1, box.left + 2, box.title, box.right - box.left - 5, 0x1E );
  for ( int d = 0; d < PickRows && cursor.top() + d < cursor.count(); d++ ) {
    int i = cursor.top() + d;
    unsigned char a = ( i == cursor.sel() ) ? 0x30 : 0x1F;
    int pos = canvas.put( Canvas::at( d + 2, box.left + 1 ), ' ', a );
    pos = canvas.text( pos, picks.items[i], box.right - box.left - 3, a );
    canvas.fillTo( pos, Canvas::at( d + 2, box.right ), a );
  }
  frame_.footer( Keys );
  frame_.end();
}
