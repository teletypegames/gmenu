/*
   The games of the game directory, in the order the folder scan found them; a game is an
   index into them. ordered() gives them in the sort order. The games are kept in blocks
   and their records in a pool of chunks, both allocated as they are needed, so the number
   of games is limited only by the memory (and MaxGames, what a 16-bit index array holds).
*/

#ifndef GMENU_MODEL_GAME_LIBRARY_H
#define GMENU_MODEL_GAME_LIBRARY_H

#include "game.h"
#include "sort_order.h"

class GameLibrary {
public:
  enum { MaxGames = 16000, BlockGames = 128, ChunkSize = 16384, MaxChunks = 40 };

  GameLibrary();
  ~GameLibrary();
  /* false when there is no memory for the first games */
  bool allocate();
  /* No games and no records any more (the memory is kept for the next ones). */
  void clear();
  int count() const;
  Game &at( int g );
  const Game &at( int g ) const;
  /* A new, cleared game: its index; -1 when there is no memory for it. */
  int add( const char *folder );
  /* The i-th game in the sort order. */
  int ordered( int i ) const;
  /* The game of a folder name, or -1. */
  int find( const char *folder ) const;
  void sort( int order );
  /* A record buffer from the pool; NULL when there is no memory. */
  char *allocRecord( unsigned len );

private:
  enum { MaxBlocks = MaxGames / BlockGames };

  GameLibrary( const GameLibrary & );
  GameLibrary &operator=( const GameLibrary & );
  bool growOrder();

  Game *blocks_[MaxBlocks];
  char *chunks_[MaxChunks];
  int chunk_;          /* the chunk records come from */
  unsigned chunkUsed_; /* its bytes in use */
  int *order_;
  int orderCapacity_;
  int count_;
};

#endif
