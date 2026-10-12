/*
   The games of the game directory, in the order the folder scan found them; a game is an
   index into them. ordered() gives them in the sort order.
*/

#ifndef GMENU_MODEL_GAME_LIBRARY_H
#define GMENU_MODEL_GAME_LIBRARY_H

#include "game.h"
#include "sort_order.h"

class GameLibrary {
public:
  enum { Capacity = 600, RecordsSize = 60000 };

  GameLibrary();
  ~GameLibrary();
  /* false when there is no memory for the games */
  bool allocate();
  int count() const;
  bool isFull() const;
  Game &at( int g );
  const Game &at( int g ) const;
  /* A new, cleared game: its index. */
  int add( const char *folder );
  /* The i-th game in the sort order. */
  int ordered( int i ) const;
  /* The game of a folder name, or -1. */
  int find( const char *folder ) const;
  void sort( int order );
  /* A record buffer from the pool; NULL when the pool is full. */
  char *allocRecord( unsigned len );

private:
  GameLibrary( const GameLibrary & );
  GameLibrary &operator=( const GameLibrary & );

  Game *games_;
  char *records_;
  unsigned recordsUsed_;
  int count_;
  int order_[Capacity];
};

#endif
