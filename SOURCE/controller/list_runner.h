/*
   A list on the screen: the arrows, paging, Home / End, the wheel and the mouse move the
   selection, Enter (or a click) selects, Esc leaves; the other keys go back to the
   caller. The page draws the list and names its items.
*/

#ifndef GMENU_CONTROLLER_LIST_RUNNER_H
#define GMENU_CONTROLLER_LIST_RUNNER_H

#include "../lib/ui/list_cursor.h"

class MenuInput;

class ListPage {
public:
  virtual void draw( const ListCursor &cursor ) = 0;
  /* What a letter jumps to. */
  virtual const char *label( int i ) = 0;
};

/* How a list differs from the others. */
struct ListBehavior {
  int firstRow;            /* the screen row of the first item */
  bool typingSearches;     /* letters go to the caller (the search), not jump */
  bool gameList;           /* a click on the selected item opens it, a double click
                                   is InputDoubleClick */
  int paneEnd;             /* clicks and the wheel left of this column are for the
                                   caller (0: none) */
  bool shiftPagesToCaller; /* Shift+PgUp / PgDn go to the caller */
};

enum ListResult { ListSelected, ListEsc, ListKey };

class ListRunner {
public:
  explicit ListRunner( MenuInput &input );
  /* Runs the list until an item is selected, Esc, or a key for the caller (key). */
  ListResult run( ListPage &page, const ListBehavior &behavior, ListCursor &cursor, unsigned &key );

private:
  enum Step { StepWait, StepRedraw, StepSelected, StepEsc, StepKey };

  Step handle( unsigned &key );
  Step handleKey( unsigned key );
  Step handleWheel( unsigned key );
  Step handleClick( unsigned &key );
  Step moveTo( int index );
  Step selected();
  void jumpTo( char c );

  MenuInput &input_;
  ListPage *page_;
  const ListBehavior *behavior_;
  ListCursor *cursor_;
};

#endif
