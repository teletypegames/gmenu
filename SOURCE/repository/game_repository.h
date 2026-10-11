#ifndef GMENU_REPOSITORY_GAME_REPOSITORY_H
#define GMENU_REPOSITORY_GAME_REPOSITORY_H

class GameLibrary;

/* The games of the game directory. */
class GameRepository {
public:
  virtual void load( GameLibrary &library ) = 0;
};

#endif
