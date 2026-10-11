/*
   What every list page has: the header row (its title and "selected/count") and the
   footer row with the keys. The mouse cursor is hidden while a page is drawn.
*/

#ifndef GMENU_VIEW_FRAME_H
#define GMENU_VIEW_FRAME_H

class Canvas;
class ListCursor;
class Mouse;

class Frame {
public:
  Frame( Canvas &canvas, Mouse &mouse );
  Canvas &canvas();
  void begin( const char *title, const ListCursor &cursor );
  void beginPlain();
  /* '~' toggles the highlight. */
  void footer( const char *keys );
  void searchFooter( const char *search );
  void end();

private:
  Canvas &canvas_;
  Mouse &mouse_;
};

#endif
