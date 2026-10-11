#ifndef GMENU_REPOSITORY_STATS_REPOSITORY_H
#define GMENU_REPOSITORY_STATS_REPOSITORY_H

class GameLibrary;

/* How many times and when the games were started. */
class StatsRepository {
public:
  virtual void load( GameLibrary &library ) = 0;
  virtual void save( const GameLibrary &library ) = 0;
};

#endif
