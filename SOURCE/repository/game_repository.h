#ifndef GMENU_REPOSITORY_GAME_REPOSITORY_H
#define GMENU_REPOSITORY_GAME_REPOSITORY_H

class GameLibrary;
class Progress;

/* The games of the game directory. */
class GameRepository {
public:
  /* The games of the last scan, without looking at the folders; a scan when there is none
     yet. */
  virtual void load( GameLibrary &library, Progress &progress ) = 0;
  /* The folders scanned again (the files of a game are read again when they changed);
     progress counts the folders. */
  virtual void scan( GameLibrary &library, Progress &progress ) = 0;
  /* The files of one game checked again: true when they changed and were read again. */
  virtual bool recheck( GameLibrary &library, int game ) = 0;
};

#endif
