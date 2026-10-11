#ifndef GMENU_REPOSITORY_FAVORITE_REPOSITORY_H
#define GMENU_REPOSITORY_FAVORITE_REPOSITORY_H

class GameLibrary;

/* Which games are favorites. */
class FavoriteRepository {
public:
  virtual void load( GameLibrary &library ) = 0;
  virtual void save( const GameLibrary &library ) = 0;
};

#endif
