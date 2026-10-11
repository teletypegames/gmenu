#ifndef GMENU_REPOSITORY_GAME_FILES_REPOSITORY_H
#define GMENU_REPOSITORY_GAME_FILES_REPOSITORY_H

struct FileList;

/* The files of a game's folder: only the executables, or all of them. */
class GameFilesRepository {
public:
  virtual void list( const char *folder, bool all, FileList &files ) = 0;
};

#endif
