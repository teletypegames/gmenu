#include <string.h>

#include "frame.h"
#include "game_list_view.h"
#include "../lib/core/str.h"
#include "../lib/ui/canvas.h"
#include "../lib/ui/list_cursor.h"
#include "../model/game_library.h"
#include "../model/locations.h"
#include "../model/session.h"
#include "../model/visible_games.h"

enum {
  Cols = Canvas::Cols,
  ColTitle = 1,
  WidthTitle = 44,
  ColGenre = 46,
  WidthGenre = 15,
  ColYear = 62,
  WidthYear = 4,
  ColFolder = 67,
  WidthFolder = 12
};
static const unsigned char AttrHeads = 0x1E;
static const unsigned char AttrRow = 0x1F;
static const unsigned char AttrSelected = 0x30;

static const char NoMatch[] = "No games match the search / filters.  (Esc: clear, F5: clear filters)";
static const char Keys[] = "~Type~ Search ~Ctrl+Enter~ Play ~F1~ Help ~F2-4~ Filter ~F6/7~ Fav ~F9~ Sort ~F10~ Random";

GameListView::GameListView( Frame &frame, const GameLibrary &library, const Session &session,
                            const VisibleGames &visible, const Locations &where )
  : frame_( frame ), library_( library ), session_( session ), visible_( visible ), where_( where ) {}

static void append( char *out, const char *label, const char *value ) {
  strcat( out, label );
  strcat( out, value );
}

void GameListView::title( char *out ) const {
  const FilterCriteria &c = session_.criteria;
  strcpy( out, "GMenu Launcher  (" );
  strcat( out, where_.gameDir );
  strcat( out, ")" );
  if ( c.filtersActive() || c.searchLen ) {
    char *d = out + strlen( out );
    strcpy( d, "  Found: " );
    d = numStr( d + strlen( d ), visible_.count );
    strcpy( d, " of " );
    numStr( d + strlen( d ), library_.count() );
  }
  if ( session_.sortOrder ) append( out, "  Sort: ", sortOrderName( session_.sortOrder ) );
  if ( c.favoritesOnly ) strcat( out, "  Favorites" );
  if ( c.noMetadataOnly ) strcat( out, "  No METADATA.INI" );
  if ( c.genre[0] ) append( out, "  Genre: ", c.genre );
  if ( c.year[0] ) append( out, "  Year: ", c.year );
  if ( c.players[0] ) append( out, "  Players: ", c.players );
}

static int column( Canvas &canvas, int pos, int rowStart, int col, const char *s, int width, unsigned char attr ) {
  pos = canvas.fillTo( pos, rowStart + col, attr );
  return canvas.text( pos, s, width, attr );
}

void GameListView::drawHeaderRow() {
  Canvas &canvas = frame_.canvas();
  int pos = Cols, rowStart = Cols;
  pos = column( canvas, pos, rowStart, ColTitle, "Title", WidthTitle, AttrHeads );
  pos = column( canvas, pos, rowStart, ColGenre, "Genre", WidthGenre, AttrHeads );
  pos = column( canvas, pos, rowStart, ColYear, "Year", WidthYear, AttrHeads );
  pos = column( canvas, pos, rowStart, ColFolder, "Folder", WidthFolder, AttrHeads );
  canvas.fillTo( pos, rowStart + Cols, AttrHeads );
}

void GameListView::drawGameRow( int row, const Game &game, bool selected ) {
  Canvas &canvas = frame_.canvas();
  unsigned char a = selected ? AttrSelected : AttrRow;
  int rowStart = Canvas::at( row, 0 );
  int pos = rowStart;
  pos = column( canvas, pos, rowStart, ColTitle, game.title, WidthTitle, a );
  pos = column( canvas, pos, rowStart, ColGenre, game.field( FieldGenre ), WidthGenre, selected ? a : 0x1B );
  pos = column( canvas, pos, rowStart, ColYear, game.field( FieldYear ), WidthYear, selected ? a : 0x1E );
  pos = column( canvas, pos, rowStart, ColFolder, game.folder, WidthFolder, selected ? a : 0x17 );
  if ( game.isFavorite() ) {
    pos = canvas.fillTo( pos, rowStart + 79, a );
    pos = canvas.put( pos, '*', ( a & 0xF0 ) | 0x0E );
  }
  canvas.fillTo( pos, rowStart + Cols, a );
}

void GameListView::drawRows( const ListCursor &cursor ) {
  drawHeaderRow();
  for ( int d = 0; d < cursor.rows(); d++ ) {
    int i = cursor.top() + d;
    if ( i < cursor.count() ) {
      drawGameRow( 2 + d, library_.at( visible_.games[i] ), i == cursor.sel() );
    }
    else {
      int rowStart = Canvas::at( 2 + d, 0 );
      frame_.canvas().fillTo( rowStart, rowStart + Cols, AttrRow );
    }
  }
  if ( cursor.count() == 0 ) frame_.canvas().textAt( 3, 3, NoMatch, 70, 0x17 );
}

void GameListView::draw( const ListCursor &cursor ) {
  char header[200];
  title( header );
  frame_.begin( header, cursor );
  drawRows( cursor );
  if ( session_.criteria.searchLen == 0 ) frame_.footer( Keys );
  else frame_.searchFooter( session_.criteria.search );
  frame_.end();
}
