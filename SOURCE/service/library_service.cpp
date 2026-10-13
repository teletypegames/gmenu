#include <string.h>

#include "library_service.h"
#include "../lib/core/str.h"
#include "../lib/platform/clock.h"
#include "../model/game_library.h"
#include "../model/session.h"
#include "../model/visible_games.h"
#include "../repository/favorite_repository.h"
#include "../repository/session_repository.h"

LibraryService::LibraryService( GameLibrary &library, Session &session, VisibleGames &visible,
                                FavoriteRepository &favorites, SessionRepository &sessions, Clock &clock )
  : library_( library ), session_( session ), visible_( visible ), favorites_( favorites ), sessions_( sessions ),
    clock_( clock ), seed_( 0 ) {}

void LibraryService::sort() {
  library_.sort( session_.sortOrder );
}

void LibraryService::restore() {
  sessions_.load( session_ );
  sort();
  session_.criteria.search[session_.criteria.searchLen] = 0;
  applyFilter();
  for ( int i = 0; i < visible_.count; i++ ) {
    if ( cmpi( library_.at( visible_.games[i] ).folder, session_.selected ) == 0 ) {
      visible_.sel = i;
      visible_.center();
      break;
    }
  }
}

void LibraryService::saveSession() {
  session_.selected[0] = 0;
  if ( visible_.sel < visible_.count ) strcpy( session_.selected, library_.at( visible_.games[visible_.sel] ).folder );
  sessions_.save( session_ );
}

void LibraryService::applyFilter() {
  visible_.count = 0;
  int n = library_.count();
  while ( !visible_.reserve( n ) ) n /= 2;
  for ( int i = 0; i < library_.count() && visible_.count < n; i++ ) {
    int g = library_.ordered( i );
    if ( session_.criteria.matches( library_.at( g ) ) ) visible_.games[visible_.count++] = g;
  }
}

void LibraryService::reorder() {
  sort();
  refilter();
}

void LibraryService::refilter() {
  int keep = visible_.current();
  applyFilter();
  visible_.sel = 0;
  visible_.select( keep );
  visible_.center();
}

void LibraryService::addToSearch( char c ) {
  if ( session_.criteria.addToSearch( c ) ) refilter();
}

void LibraryService::backspace() {
  if ( session_.criteria.backspace() ) refilter();
}

void LibraryService::clearSearch() {
  session_.criteria.clearSearch();
  refilter();
}

void LibraryService::clearFilters() {
  session_.criteria.clearFilters();
  refilter();
}

void LibraryService::toggleFavoritesOnly() {
  session_.criteria.favoritesOnly ^= 1;
  refilter();
}

void LibraryService::toggleNoMetadataOnly() {
  session_.criteria.noMetadataOnly ^= 1;
  refilter();
}

void LibraryService::nextSortOrder() {
  if ( ++session_.sortOrder >= SortOrderCount ) session_.sortOrder = 0;
  sort();
  refilter();
}

void LibraryService::toggleFavorite( int game ) {
  library_.at( game ).flags ^= GameFavorite;
  favorites_.save( library_ );
}

void LibraryService::selectRandom() {
  unsigned g;
  if ( visible_.count <= 1 ) return;
  do {
    seed_ = seed_ * 25173u + 13849u + clock_.ticks();
    g = (unsigned)( ( (unsigned long)( seed_ & 0xFFFFu ) * visible_.count ) >> 16 );
  } while ( (int)g == visible_.sel );
  visible_.sel = g;
  visible_.center();
}
