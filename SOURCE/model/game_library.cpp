#include <stdlib.h>
#include <string.h>

#include "game_library.h"
#include "../lib/core/sort.h"
#include "../lib/core/str.h"

const char *sortOrderName( int order ) {
  static const char *const names[SortOrderCount] = { "title", "year", "genre", "most played" };
  return names[order];
}

GameLibrary::GameLibrary() : games_( NULL ), count_( 0 ) {}

GameLibrary::~GameLibrary() {
  free( games_ );
}

bool GameLibrary::allocate() {
  games_ = (Game *)calloc( Capacity, sizeof( Game ) );
  return games_ != NULL;
}

int GameLibrary::count() const {
  return count_;
}

bool GameLibrary::isFull() const {
  return count_ >= Capacity;
}

Game &GameLibrary::at( int g ) {
  return games_[g];
}

const Game &GameLibrary::at( int g ) const {
  return games_[g];
}

int GameLibrary::add( const char *folder ) {
  int g = count_++;
  order_[g] = g;
  memcpy( games_[g].folder, folder, FolderSize );
  return g;
}

int GameLibrary::ordered( int i ) const {
  return order_[i];
}

int GameLibrary::find( const char *folder ) const {
  for ( int i = 0; i < count_; i++ ) {
    if ( cmpi( games_[order_[i]].folder, folder ) == 0 ) return order_[i];
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
