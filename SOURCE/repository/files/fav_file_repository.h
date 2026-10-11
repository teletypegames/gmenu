#ifndef GMENU_REPOSITORY_FAV_FILE_REPOSITORY_H
#define GMENU_REPOSITORY_FAV_FILE_REPOSITORY_H

#include "../favorite_repository.h"

class FileSystem;
struct Locations;

/* GMENU.FAV: the folder names of the favorites, one per line, in the sort order. */
class FavFileRepository : public FavoriteRepository {
public:
  FavFileRepository( FileSystem &fs, const Locations &where );
  void load( GameLibrary &library );
  void save( const GameLibrary &library );

private:
  FileSystem &fs_;
  char path_[96];
};

#endif
