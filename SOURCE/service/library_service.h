/*
   The game list: the search, the filters, the sort order, the favorites, and the
   selected game, which a new search or filter keeps when it is still in the list.
*/

#ifndef GMENU_SERVICE_LIBRARY_SERVICE_H
#define GMENU_SERVICE_LIBRARY_SERVICE_H

class Clock;
class FavoriteRepository;
class GameLibrary;
class SessionRepository;
struct Session;
struct VisibleGames;

class LibraryService {
public:
  LibraryService( GameLibrary &library, Session &session, VisibleGames &visible, FavoriteRepository &favorites,
                  SessionRepository &sessions, Clock &clock );
  /* The session of the last run: its sort order, filters and selected game. */
  void restore();
  void saveSession();

  /* The list again after the filters changed, keeping the selected game (centered). */
  void refilter();
  /* Sorted and filtered again (after a game changed), keeping the selected game. */
  void reorder();
  /* The list again, the selection staying where it is. */
  void applyFilter();
  void addToSearch( char c );
  void backspace();
  void clearSearch();
  void clearFilters();
  void toggleFavoritesOnly();
  void toggleNoMetadataOnly();
  void nextSortOrder();
  void toggleFavorite( int game );
  /* A random other game of the list, centered. */
  void selectRandom();

private:
  void sort();

  GameLibrary &library_;
  Session &session_;
  VisibleGames &visible_;
  FavoriteRepository &favorites_;
  SessionRepository &sessions_;
  Clock &clock_;
  unsigned seed_;
};

#endif
