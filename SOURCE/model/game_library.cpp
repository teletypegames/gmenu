#include <stdlib.h>
#include <string.h>

#include "game_library.h"
#include "../lib/core/sort.h"
#include "../lib/core/str.h"

const char *sortOrderName( int order ) {
  static const char *const names[SortOrderCount] = { "title", "year", "genre", "most played" };
  return names[order];
}

GameLibrary::GameLibrary() : chunk_( 0 ), chunkUsed_( 0 ), order_( NULL ), orderCapacity_( 0 ), count_( 0 ) {
  memset( blocks_, 0, sizeof( blocks_ ) );
  memset( chunks_, 0, sizeof( chunks_ ) );
}

GameLibrary::~GameLibrary() {
  for ( int b = 0; b < MaxBlocks; b++ ) free( blocks_[b] );
  for ( int c = 0; c < MaxChunks; c++ ) free( chunks_[c] );
  free( order_ );
}

bool GameLibrary::allocate() {
  blocks_[0] = (Game *)malloc( BlockGames * sizeof( Game ) );
  chunks_[0] = (char *)malloc( ChunkSize );
  return blocks_[0] != NULL && chunks_[0] != NULL && growOrder();
}

void GameLibrary::clear() {
  count_ = 0;
  chunk_ = 0;
  chunkUsed_ = 0;
}

int GameLibrary::count() const {
  return count_;
}

Game &GameLibrary::at( int g ) {
  return blocks_[g / BlockGames][g % BlockGames];
}

const Game &GameLibrary::at( int g ) const {
  return blocks_[g / BlockGames][g % BlockGames];
}

/* The order array twice as big (at most MaxGames). */
bool GameLibrary::growOrder() {
  int capacity = orderCapacity_ ? orderCapacity_ * 2 : BlockGames;
  if ( capacity > MaxGames ) capacity = MaxGames;
  if ( capacity <= orderCapacity_ ) return false;
  int *order = (int *)realloc( order_, capacity * sizeof( int ) );
  if ( order == NULL ) return false;
  order_ = order;
  orderCapacity_ = capacity;
  return true;
}

int GameLibrary::add( const char *folder ) {
  if ( count_ >= MaxGames ) return -1;
  if ( count_ == orderCapacity_ && !growOrder() ) return -1;
  Game *&block = blocks_[count_ / BlockGames];
  if ( block == NULL ) block = (Game *)malloc( BlockGames * sizeof( Game ) );
  if ( block == NULL ) return -1;
  int g = count_++;
  order_[g] = g;
  Game &game = at( g );
  memset( &game, 0, sizeof( game ) );
  memcpy( game.folder, folder, FolderSize );
  return g;
}

char *GameLibrary::allocRecord( unsigned len ) {
  if ( len > ChunkSize ) return NULL;
  if ( chunkUsed_ + len > ChunkSize ) {
    if ( chunk_ + 1 >= MaxChunks ) return NULL;
    if ( chunks_[chunk_ + 1] == NULL ) chunks_[chunk_ + 1] = (char *)malloc( ChunkSize );
    if ( chunks_[chunk_ + 1] == NULL ) return NULL;
    chunk_++;
    chunkUsed_ = 0;
  }
  char *p = chunks_[chunk_] + chunkUsed_;
  chunkUsed_ += len;
  return p;
}

int GameLibrary::ordered( int i ) const {
  return order_[i];
}

int GameLibrary::find( const char *folder ) const {
  for ( int i = 0; i < count_; i++ ) {
    if ( cmpi( at( order_[i] ).folder, folder ) == 0 ) return order_[i];
  }
  return -1;
}

class GameOrder {
public:
  GameOrder( const GameLibrary &library, int order ) : library_( library ), order_( order ) {}
  int compare( int a, int b ) const;

private:
  int byField( const Game &x, const Game &y, int field ) const;
  int byPlays( const Game &x, const Game &y ) const;

  const GameLibrary &library_;
  int order_;
};

/* By the field (empty last), then by title. */
int GameOrder::byField( const Game &x, const Game &y, int field ) const {
  const char *a = x.field( field );
  const char *b = y.field( field );
  if ( !*a || !*b ) {
    if ( *a ) return -1;
    if ( *b ) return 1;
    return compareTitles( x, y );
  }
  int r = cmpi( a, b );
  return r ? r : compareTitles( x, y );
}

/* Most starts first, then the latest start, then the title. */
int GameOrder::byPlays( const Game &x, const Game &y ) const {
  if ( x.plays != y.plays ) return x.plays > y.plays ? -1 : 1;
  if ( x.last != y.last ) return x.last > y.last ? -1 : 1;
  return compareTitles( x, y );
}

int GameOrder::compare( int a, int b ) const {
  const Game &x = library_.at( a );
  const Game &y = library_.at( b );
  if ( order_ == SortYear ) return byField( x, y, FieldYear );
  if ( order_ == SortGenre ) return byField( x, y, FieldGenre );
  if ( order_ == SortPlays ) return byPlays( x, y );
  return compareTitles( x, y );
}

void GameLibrary::sort( int order ) {
  GameOrder byOrder( *this, order );
  shellSortBy( order_, count_, byOrder );
}
