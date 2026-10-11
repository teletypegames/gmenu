/*
   The game page: the game's data and description on the left, the files of its folder
   on the right (the RUN file marked).
*/

#ifndef GMENU_VIEW_DETAIL_VIEW_H
#define GMENU_VIEW_DETAIL_VIEW_H

class Canvas;
class Frame;
class ListCursor;
struct FileList;
struct Game;
struct Locations;

class DetailView {
public:
  DetailView( Frame &frame, const Locations &where );
  /* scroll: the description lines scrolled off the top. The count of all wrapped lines. */
  int draw( const ListCursor &cursor, const Game &game, const FileList &files, bool showAll, int scroll );

private:
  int drawData( const Game &game, int scroll );
  void infoLine( int row, const Game &game, int field, const char *label );
  void drawFiles( const ListCursor &cursor, const Game &game, const FileList &files, bool showAll );

  Frame &frame_;
  const Locations &where_;
};

/* Word-wrapped text from row, col; '\n' breaks the line. The first skip lines are not
   shown: the count of all lines. */
int wrapText( Canvas &canvas, const char *s, int row, int col, int rows, int width, unsigned char attr, int skip );

#endif
