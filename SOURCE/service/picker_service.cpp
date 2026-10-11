#include <string.h>

#include "picker_service.h"
#include "../lib/core/sort.h"
#include "../lib/core/str.h"
#include "../model/filter_criteria.h"
#include "../model/game_library.h"
#include "../model/pick_list.h"

PickerService::PickerService( const GameLibrary &library ) : library_( library ) {}

/* Every player count 1..16 a game's range covers ("a+": only a). */
void PickerService::addPlayerCounts( PickList &picks ) {
  char seen[MaxPlayerCount + 1];
  memset( seen, 0, sizeof( seen ) );
  for ( int i = 0; i < library_.count(); i++ ) {
    const char *p = library_.at( library_.ordered( i ) ).field( FieldPlayers );
    unsigned a, b;
    if ( !parseRange( &p, &a, &b ) ) continue;
    if ( b == 0xFFFF ) b = a;
    if ( b > MaxPlayerCount ) b = MaxPlayerCount;
    for ( ; a <= b; a++ ) seen[a] = 1;
  }
  for ( int n = 1; n <= MaxPlayerCount; n++ ) {
    char num[6];
    if ( !seen[n] ) continue;
    numStr( num, n );
    picks.addCopy( num );
  }
}

static bool isYear( const char *s ) {
  for ( int i = 0; i < 4; i++ ) {
    if ( s[i] < '0' || s[i] > '9' ) return false;
  }
  return true;
}

/* The years are items 1..count-1, sorted: their decades ("1990s") go in front of them. */
void PickerService::addDecades( PickList &picks ) {
  int years = picks.count;
  for ( int i = 1; i < years; i++ ) {
    const char *s = picks.items[i];
    if ( !isYear( s ) ) continue;
    char decade[6];
    memcpy( decade, s, 3 );
    decade[3] = '0';
    decade[4] = 's';
    decade[5] = 0;
    if ( picks.count == years || cmpi( picks.items[picks.count - 1], decade ) ) picks.addCopy( decade );
  }
  for ( int j = years, k = 1; j < picks.count; j++, k++ ) {
    const char *d = picks.items[j];
    memmove( &picks.items[k + 1], &picks.items[k], ( j - k ) * sizeof( picks.items[0] ) );
    picks.items[k] = d;
  }
}

int PickerService::filterValues( int field, const char *current, PickList &picks ) {
  picks.clear();
  picks.addRef( "(all)" );
  if ( field == FieldPlayers ) addPlayerCounts( picks );
  else {
    for ( int i = 0; i < library_.count(); i++ ) {
      const char *v = library_.at( library_.ordered( i ) ).field( field );
      if ( *v && picks.find( v, 1 ) < 0 ) picks.addCopy( v );
    }
    shellSort( picks.items + 1, picks.count - 1, cmpi );
    if ( field == FieldYear ) addDecades( picks );
  }
  int sel = *current ? picks.find( current, 1 ) : -1;
  return sel < 0 ? 0 : sel;
}

int PickerService::recentGames( const RecentList &recent, PickList &picks ) {
  picks.clear();
  for ( int i = 0; i < RecentList::Size; i++ ) {
    if ( !recent.folders[i][0] ) continue;
    int g = library_.find( recent.folders[i] );
    if ( g < 0 ) continue;
    picks.games[picks.count] = g;
    picks.addRef( library_.at( g ).title );
  }
  return picks.count;
}
