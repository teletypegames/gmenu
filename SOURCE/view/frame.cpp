#include "frame.h"
#include "../lib/platform/mouse.h"
#include "../lib/ui/canvas.h"
#include "../lib/ui/list_cursor.h"

enum { Cols = Canvas::Cols, Rows = Canvas::Rows, FooterRow = 24 };
static const unsigned char AttrBar = 0x70;

Frame::Frame( Canvas &canvas, Mouse &mouse ) : canvas_( canvas ), mouse_( mouse ) {}

Canvas &Frame::canvas() {
  return canvas_;
}

void Frame::beginPlain() {
  mouse_.hide();
}

void Frame::begin( const char *title, const ListCursor &cursor ) {
  mouse_.hide();
  int pos = canvas_.put( 0, ' ', AttrBar );
  pos = canvas_.text( pos, title, 64, AttrBar );
  pos = canvas_.fillTo( pos, 68, AttrBar );
  pos = canvas_.number( pos, cursor.count() ? cursor.sel() + 1 : cursor.sel(), AttrBar );
  pos = canvas_.put( pos, '/', AttrBar );
  pos = canvas_.number( pos, cursor.count(), AttrBar );
  canvas_.fillTo( pos, Cols, AttrBar );
}

void Frame::footer( const char *keys ) {
  int pos = canvas_.put( FooterRow * Cols, ' ', AttrBar );
  pos = canvas_.footer( pos, keys );
  canvas_.fillTo( pos, Cols * Rows, AttrBar );
}

void Frame::searchFooter( const char *search ) {
  int pos = canvas_.put( FooterRow * Cols, ' ', AttrBar );
  pos = canvas_.text( pos, "Search: ", 20, 0x1E );
  pos = canvas_.text( pos, search, 30, 0x1F );
  pos = canvas_.put( pos, '_', 0x1F );
  pos = canvas_.text( pos, "   (Backspace: delete, Esc: clear search)", 79, 0x1E );
  canvas_.fillTo( pos, Cols * Rows, 0x1E );
}

void Frame::end() {
  canvas_.flush();
  mouse_.show();
}
