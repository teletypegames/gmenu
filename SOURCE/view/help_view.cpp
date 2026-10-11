#include <stddef.h>

#include "frame.h"
#include "help_view.h"
#include "layout.h"
#include "../lib/ui/canvas.h"

enum { KeyCapWidth = 16 };
static const unsigned char AttrFrame = 0x13;
static const unsigned char AttrTitle = 0x1E;
static const unsigned char AttrCap = 0x30;
static const unsigned char AttrText = 0x1F;

/* A section title (key NULL), an empty row (both NULL), or a key and what it does. */
static const struct {
  const char *key, *desc;
} Lines[] = { { NULL, "Game list" },
              { "Enter, click", "open the game page" },
              { "Ctrl+Enter", "play (or double click)" },
              { "Ctrl+S", "run the setup" },
              { "Ctrl+E", "edit METADATA.INI" },
              { "Typing", "search" },
              { "F2  F3  F4", "filter: genre, year, players" },
              { "F5", "clear the filters" },
              { "F6  F7", "favorite, favorites only" },
              { "Shift+F7", "no METADATA.INI only" },
              { "F8", "recently played" },
              { "F9  F10", "sort order, random game" },
              { "Esc", "clear search, filters, quit" },
              { NULL, NULL },
              { NULL, "Game page" },
              { "Enter, click", "run the file" },
              { "Tab", "all files / executables" },
              { "A-Z  0-9", "jump to a file" },
              { "Ctrl+PgUp/Dn", "scroll the description" },
              { "F6", "favorite" },
              { "Ctrl+E", "edit METADATA.INI" },
              { "Esc  \x1B", "back to the list" } };

static const char Keys[] = "~\x18\x19 PgUp PgDn~ Scroll  ~Esc~ Close";

HelpView::HelpView( Frame &frame ) : frame_( frame ) {}

int HelpView::lineCount() {
  return sizeof( Lines ) / sizeof( Lines[0] );
}

void HelpView::draw( int top ) {
  Canvas &canvas = frame_.canvas();
  frame_.beginPlain();
  canvas.box( 1, 4, 23, 75, AttrFrame );
  canvas.boxTitle( 1, 6, "Keys", 20, AttrTitle );
  for ( int i = top; i < lineCount() && i - top < HelpRows; i++ ) {
    int row = 2 + i - top;
    int pos;
    if ( Lines[i].key ) {
      pos = canvas.put( Canvas::at( row, 7 ), ' ', AttrCap );
      pos = canvas.text( pos, Lines[i].key, KeyCapWidth - 2, AttrCap );
      pos = canvas.fillTo( pos, Canvas::at( row, 7 + KeyCapWidth ), AttrCap );
      canvas.text( pos + 2, Lines[i].desc, 44, AttrText );
    }
    else if ( Lines[i].desc ) {
      pos = canvas.text( Canvas::at( row, 7 ), Lines[i].desc, 30, AttrTitle );
      pos = canvas.put( pos, ' ', AttrTitle );
      do pos = canvas.put( pos, (char)0xC4, AttrFrame );
      while ( pos < Canvas::at( row, 73 ) );
    }
  }
  if ( top ) canvas.put( Canvas::at( 1, 71 ), 0x18, AttrFrame );
  if ( lineCount() - HelpRows > top ) canvas.put( Canvas::at( 23, 71 ), 0x19, AttrFrame );
  frame_.footer( Keys );
  frame_.end();
}
