/*
   The games the search and the filters let through, in the sort order, and the selected
   one of them with the first visible row. The list grows with the library (reserve()).
*/

#ifndef GMENU_MODEL_VISIBLE_GAMES_H
#define GMENU_MODEL_VISIBLE_GAMES_H

#include "game_library.h"

struct VisibleGames {
  int *games;
  int count;
  int sel, top;
  int rows; /* the rows of the list on the screen */

  VisibleGames();
  ~VisibleGames();
  /* Room for n games; false when there is no memory for it. */
  bool reserve( int n );
  /* The selected game, or -1 when the list is empty. */
  int current() const;
  /* Selects the game when it is in the list; false when it is not. */
  bool select( int game );
  /* The selection in the middle of the rows, as far as the top allows. */
  void center();

private:
  VisibleGames( const VisibleGames & );
  VisibleGames &operator=( const VisibleGames & );

  int capacity_;
};

#endif
