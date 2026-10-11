#include <string.h>

#include "../../lib/testing/host/fake_clock.h"
#include "../../lib/testing/host/minitest.h"
#include "../../model/game_library.h"
#include "../../model/metadata.h"
#include "../../model/pick_list.h"
#include "../../model/session.h"
#include "../../model/visible_games.h"
#include "../../repository/favorite_repository.h"
#include "../../repository/session_repository.h"
#include "../../service/library_service.h"
#include "../../service/picker_service.h"

class NoFavorites : public FavoriteRepository {
public:
  NoFavorites() : saves( 0 ) {}
  void load( GameLibrary & ) {}
  void save( const GameLibrary & ) { saves++; }
  int saves;
};

class SavedSession : public SessionRepository {
public:
  SavedSession() : present( false ) { memset( &saved, 0, sizeof( saved ) ); }
  bool load( Session &session ) {
    if ( present ) session = saved;
    return present;
  }
  void save( const Session &session ) {
    saved = session;
    present = true;
  }
  Session saved;
  bool present;
};

/* Five games: Alpha 1991 Action, Bravo 1995 Sports, Charlie 1990 Action, Delta 2001, Echo. */
struct Shelf {
  GameLibrary library;
  Session session;
  VisibleGames visible;
  NoFavorites favorites;
  SavedSession sessions;
  FakeClock clock;
  LibraryService games;

  Shelf() : session(), visible(), games( library, session, visible, favorites, sessions, clock ) {
    static const char *const inis[] = { "NAME=Alpha\nYEAR=1991\nGENRE=Action\n",
                                        "NAME=Bravo\nYEAR=1995\nGENRE=Sports\n",
                                        "NAME=Charlie\nYEAR=1990\nGENRE=Action\n", "NAME=Delta\nYEAR=2001\n",
                                        "NAME=Echo\n" };
    static const char *const folders[] = { "A", "B", "C", "D", "E" };
    library.allocate();
    visible.rows = 22;
    for ( int i = 0; i < 5; i++ ) {
      Game &g = library.at( library.add( folders[i] ) );
      Metadata m;
      m.parse( inis[i] );
      m.storeRecord( g );
      m.applyTitle( g );
    }
    games.restore();
  }
};

TEST( refilterKeepsTheSelectedGame ) {
  Shelf s;
  s.visible.sel = 2;
  strcpy( s.session.criteria.genre, "Action" );
  s.games.refilter();
  CHECK_INT( s.visible.count, 2 );
  CHECK_INT( s.visible.current(), 2 );
  CHECK_INT( s.visible.sel, 1 );
  s.games.addToSearch( 'a' );
  s.games.addToSearch( 'l' );
  CHECK_INT( s.visible.count, 1 );
  CHECK_INT( s.visible.sel, 0 );
  s.games.clearSearch();
  s.games.clearFilters();
  CHECK_INT( s.visible.count, 5 );
}

TEST( sessionRestoresTheSelection ) {
  Shelf s;
  s.visible.sel = 3;
  s.session.sortOrder = SortYear;
  s.games.saveSession();
  CHECK_STR( s.sessions.saved.selected, "D" );
  Session fresh;
  memset( &fresh, 0, sizeof( fresh ) );
  s.session = fresh;
  s.visible.sel = 0;
  s.games.restore();
  CHECK_INT( s.visible.current(), 3 );
  CHECK_INT( s.library.ordered( 0 ), 2 );
}

TEST( nextSortOrderWrapsAround ) {
  Shelf s;
  for ( int i = 0; i < SortOrderCount; i++ ) s.games.nextSortOrder();
  CHECK_INT( s.session.sortOrder, SortTitle );
}

TEST( favoritesAreSaved ) {
  Shelf s;
  s.games.toggleFavorite( 1 );
  CHECK( s.library.at( 1 ).isFavorite() );
  CHECK_INT( s.favorites.saves, 1 );
  s.games.toggleFavoritesOnly();
  CHECK_INT( s.visible.count, 1 );
}

TEST( randomGameIsAnotherOne ) {
  Shelf s;
  for ( int i = 0; i < 20; i++ ) {
    int before = s.visible.sel;
    s.clock.now += 55 * i;
    s.games.selectRandom();
    CHECK( s.visible.sel != before );
    CHECK( s.visible.sel >= 0 && s.visible.sel < 5 );
  }
}

TEST( yearPickerHasTheDecadesFirst ) {
  Shelf s;
  PickerService picker( s.library );
  static PickList picks;
  int sel = picker.filterValues( FieldYear, "1995", picks );
  CHECK_INT( picks.count, 7 );
  CHECK_STR( picks.items[0], "(all)" );
  CHECK_STR( picks.items[1], "1990s" );
  CHECK_STR( picks.items[2], "2000s" );
  CHECK_STR( picks.items[3], "1990" );
  CHECK_STR( picks.items[sel], "1995" );
  picker.filterValues( FieldGenre, "", picks );
  CHECK_INT( picks.count, 3 );
}

TEST( recentPickerSkipsGamesThatAreGone ) {
  Shelf s;
  PickerService picker( s.library );
  static PickList picks;
  RecentList recent;
  memset( &recent, 0, sizeof( recent ) );
  recent.add( "C" );
  recent.add( "GONE" );
  recent.add( "A" );
  CHECK_INT( picker.recentGames( recent, picks ), 2 );
  CHECK_STR( picks.items[0], "Alpha" );
  CHECK_INT( picks.games[1], 2 );
}
