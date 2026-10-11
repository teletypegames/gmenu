#include <stddef.h>

#include "edit_form_view.h"
#include "../lib/ui/canvas.h"
#include "../model/executable_list.h"
#include "../model/metadata_form.h"

enum { ValueCol = 16, ExeCols = 5, ExeRows = 5, ExeWidth = 14 };

struct FieldPlace {
  const char *label;
  int labelRow;
  int row, col, width;
  const char *hint;
};

static const FieldPlace Places[FormFieldCount] = {
  { "Name", 3, 3, ValueCol, MaxName, "The title in the game list" },
  { "Genre", 4, 4, ValueCol, MaxGenre, "For example: Action, Adventure, Racing, Sports, Strategy" },
  { "Year", 5, 5, ValueCol, MaxYear, "The year of the release, for example: 1993" },
  { "Players", 6, 6, ValueCol, MaxPlayers, "Number of players: a number (2), a range (1-4) or open-ended (2+)" },
  { "Description", 8, 9, 3, 74, "The text of the game page; \\n starts a new line" },
  { "Run", 11, 11, ValueCol, MaxCommand, "Ctrl+Enter plays it: a file of the folder, with its arguments" },
  { "Args", 12, 12, ValueCol, MaxCommand, "Arguments added to RUN, also when it is started from the file list" },
  { "Setup", 13, 13, ValueCol, MaxCommand, "Ctrl+S runs it: the setup program of the game, with its arguments" }
};

static const char Keys[] = "~Tab~/~\x18\x19~ Field  ~F2~ Save  ~Esc~ Cancel  ~Ctrl+Y~ Clear";

EditFormView::EditFormView( Canvas &canvas ) : canvas_( canvas ) {}

int EditFormView::width( int field ) {
  return Places[field].width;
}

void EditFormView::drawField( MetadataForm &form, int field, const EditCursor &cursor ) {
  const FieldPlace &place = Places[field];
  canvas_.textAt( place.labelRow, 3, place.label, ValueCol - 4, 0x1B );
  const char *v = form.value( field );
  unsigned char attr = 0x07;
  if ( field == cursor.field ) {
    v += cursor.scroll;
    attr = 0x70;
  }
  int pos = Canvas::at( place.row, place.col );
  int n = 0;
  for ( ; n < place.width && v[n]; n++ ) pos = canvas_.put( pos, v[n], attr );
  for ( ; n < place.width; n++ ) pos = canvas_.put( pos, ' ', attr );
}

void EditFormView::draw( MetadataForm &form, const char *folder, const ExecutableList &exes, const EditCursor &cursor,
                         const char *message ) {
  int pos = canvas_.text( 0, form.isExisting() ? " Edit METADATA.INI: " : " New METADATA.INI: ", 20, 0x70 );
  pos = canvas_.text( pos, folder, 60, 0x70 );
  canvas_.fillTo( pos, Canvas::Cols, 0x70 );
  canvas_.box( 1, 0, 23, 79, 0x13 );
  canvas_.boxTitle( 1, 2, "Game data", 30, 0x1E );
  for ( int f = 0; f < FormFieldCount; f++ ) drawField( form, f, cursor );
  canvas_.separator( 15, 0, 79, 0x13 );
  canvas_.boxTitle( 15, 2, "Executables", 20, 0x1E );
  if ( exes.count == 0 ) canvas_.textAt( 16, 3, "(no executables)", 20, 0x17 );
  for ( int i = 0; i < exes.count && i < ExeCols * ExeRows; i++ ) {
    canvas_.textAt( 16 + i / ExeCols, 3 + i % ExeCols * ExeWidth, exes.names[i], 12, 0x1F );
  }
  canvas_.separator( 21, 0, 79, 0x13 );
  if ( message ) canvas_.textAt( 22, 3, message, 74, 0x1E );
  else canvas_.textAt( 22, 3, Places[cursor.field].hint, 74, 0x17 );
  pos = canvas_.put( 24 * Canvas::Cols, ' ', 0x70 );
  pos = canvas_.footer( pos, Keys );
  canvas_.fillTo( pos, Canvas::Cols * Canvas::Rows, 0x70 );
  const FieldPlace &active = Places[cursor.field];
  canvas_.screen().moveCursor( active.row, active.col + cursor.pos - cursor.scroll );
  canvas_.flush();
}
