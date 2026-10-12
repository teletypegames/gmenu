#include <string.h>

#include "detail_view.h"
#include "frame.h"
#include "layout.h"
#include "../lib/core/str.h"
#include "../lib/ui/canvas.h"
#include "../lib/ui/list_cursor.h"
#include "../model/file_list.h"
#include "../model/game.h"
#include "../model/launch_command.h"
#include "../model/locations.h"

static const unsigned char AttrFrame = 0x13;
static const unsigned char AttrTitle = 0x1E;
static const unsigned char AttrLabel = 0x1B;
static const unsigned char AttrValue = 0x1F;
static const unsigned char AttrDim = 0x17;

static const char KeysExecutables[] =
    "~\x18\x19~ Move ~Enter~ Run ~Tab~ All files ~F1~ Help ~F6~ Fav ~Ctrl+PgUp/Dn~ Text ~Esc~ Back";
static const char KeysAllFiles[] =
    "~\x18\x19~ Move ~Enter~ Run ~Tab~ Executables ~F1~ Help ~F6~ Fav ~Ctrl+PgUp/Dn~ Text ~Esc~ Back";

DetailView::DetailView( Frame &frame, const Locations &where ) : frame_( frame ), where_( where ) {}

int wrapText( Canvas &canvas, const char *s, int row, int col, int rows, int width, unsigned char attr, int skip ) {
  int total = 0;
  for ( ;; ) {
    while ( *s == ' ' ) s++;
    if ( *s == 0 ) return total;
    int n = 0, space = 0;
    for ( ;; ) {
      char c = s[n];
      if ( c == 0 || c == '\n' ) break;
      if ( n >= width ) {
        if ( c != ' ' && space ) n = space;
        break;
      }
      if ( c == ' ' ) space = n;
      n++;
    }
    if ( total >= skip && total - skip < rows ) {
      int pos = Canvas::at( row + total - skip, col );
      for ( int i = 0; i < n; i++ ) pos = canvas.put( pos, s[i], attr );
    }
    s += n;
    total++;
    if ( *s == '\n' ) s++;
  }
}

void DetailView::infoLine( int row, const Game &game, int field, const char *label ) {
  Canvas &canvas = frame_.canvas();
  canvas.textAt( row, 3, label, 10, AttrLabel );
  const char *v = game.field( field );
  if ( *v ) canvas.textAt( row, 13, v, DetailLeftEnd - 14, AttrValue );
  else canvas.textAt( row, 13, "-", DetailLeftEnd - 14, AttrDim );
}

void DetailView::drawFolderAndPlayed( const Game &game ) {
  Canvas &canvas = frame_.canvas();
  char text[96];
  canvas.textAt( 6, 3, "Folder", 10, AttrLabel );
  where_.gameFile( game.folder, "", text );
  canvas.textAt( 6, 13, text, DetailLeftEnd - 14, AttrValue );
  canvas.textAt( 7, 3, "Played", 10, AttrLabel );
  game.playedText( text );
  canvas.textAt( 7, 13, text, DetailLeftEnd - 14, AttrValue );
}

int DetailView::drawDescription( const Game &game, int scroll ) {
  Canvas &canvas = frame_.canvas();
  const char *desc = game.field( FieldDesc );
  int lines;
  if ( *desc ) lines = wrapText( canvas, desc, 9, 3, DescRows, DetailLeftEnd - 4, AttrDim, scroll );
  else lines = wrapText( canvas, "No description.", 9, 3, DescRows, DetailLeftEnd - 4, 0x18, scroll );
  if ( scroll ) canvas.put( Canvas::at( 8, DetailLeftEnd - 2 ), 0x18, AttrFrame );
  if ( lines - scroll > DescRows ) canvas.put( Canvas::at( 23, DetailLeftEnd - 2 ), 0x19, AttrFrame );
  return lines;
}

int DetailView::drawData( const Game &game, int scroll ) {
  Canvas &canvas = frame_.canvas();
  canvas.box( 1, 0, 23, DetailLeftEnd, AttrFrame );
  int pos = canvas.boxTitle( 1, 2, game.title, DetailLeftEnd - 7, AttrTitle );
  if ( game.isFavorite() ) {
    pos = canvas.put( pos, '*', AttrTitle );
    canvas.put( pos, ' ', AttrTitle );
  }
  canvas.separator( 8, 0, DetailLeftEnd, AttrFrame );
  infoLine( 3, game, FieldGenre, "Genre" );
  infoLine( 4, game, FieldYear, "Year" );
  infoLine( 5, game, FieldPlayers, "Players" );
  drawFolderAndPlayed( game );
  return drawDescription( game, scroll );
}

void DetailView::drawFiles( const ListCursor &cursor, const Game &game, const FileList &files, bool showAll ) {
  Canvas &canvas = frame_.canvas();
  canvas.box( 1, DetailLeftEnd + 1, 23, 79, AttrFrame );
  canvas.boxTitle( 1, DetailLeftEnd + 3, showAll ? "All files" : "Executables", 18, AttrTitle );
  if ( cursor.count() == 0 ) {
    canvas.textAt( 2, FileCol + 1, showAll ? "(no files)" : "(no executables)", 20, AttrDim );
    if ( !showAll ) canvas.textAt( 4, FileCol + 1, "Tab: all files", 20, AttrDim );
    return;
  }
  const char *run = game.field( FieldRun );
  for ( int d = 0; d < FileRows && cursor.top() + d < cursor.count(); d++ ) {
    int i = cursor.top() + d;
    const char *name = files.sorted[i];
    bool exec = !showAll || isExecutable( name );
    bool selected = i == cursor.sel();
    unsigned char a = selected ? 0x30 : AttrValue;
    unsigned char markAttr = 0x1A;
    char mark = ' ';
    if ( showAll && exec ) mark = 0x10;
    if ( commandRuns( run, name ) ) {
      mark = 0x10;
      markAttr = 0x1E;
    }
    unsigned char nameAttr = exec || selected ? a : AttrDim;
    int pos = canvas.put( Canvas::at( d + 2, FileCol ), mark, selected ? a : markAttr );
    pos = canvas.text( pos, name, 12, nameAttr );
    canvas.fillTo( pos, Canvas::at( d + 2, 79 ), nameAttr );
  }
}

int DetailView::draw( const ListCursor &cursor, const Game &game, const FileList &files, bool showAll, int scroll ) {
  char title[80];
  strcpy( title, game.title );
  strcat( title, "  (" );
  strcat( title, game.folder );
  strcat( title, ")" );
  frame_.begin( title, cursor );
  int lines = drawData( game, scroll );
  drawFiles( cursor, game, files, showAll );
  frame_.footer( showAll ? KeysAllFiles : KeysExecutables );
  frame_.end();
  return lines;
}
