/*
   The game list: Title / Genre / Year / Folder, a '*' after a favorite. The header
   names the game directory, the matches and the active filters.
*/

#ifndef GMENU_VIEW_GAME_LIST_VIEW_H
#define GMENU_VIEW_GAME_LIST_VIEW_H

class Frame;
class GameLibrary;
class ListCursor;
struct Locations;
struct Session;
struct VisibleGames;

class GameListView {
public:
  GameListView( Frame &frame, const GameLibrary &library, const Session &session, const VisibleGames &visible,
                const Locations &where );
  void draw( const ListCursor &cursor );

private:
  void title( char *out ) const;
  void drawRows( const ListCursor &cursor );

  Frame &frame_;
  const GameLibrary &library_;
  const Session &session_;
  const VisibleGames &visible_;
  const Locations &where_;
};

#endif
