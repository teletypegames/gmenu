#ifndef GMENU_REPOSITORY_STA_FILE_REPOSITORY_H
#define GMENU_REPOSITORY_STA_FILE_REPOSITORY_H

#include "../stats_repository.h"

class FileSystem;
struct Locations;

/* GMENU.STA: records of folder (13), starts (2), date of the last start (2, DOS format);
   the records of folders missing from the game directory are kept. */
class StaFileRepository : public StatsRepository {
public:
  StaFileRepository( FileSystem &fs, const Locations &where );
  ~StaFileRepository();
  void load( GameLibrary &library );
  void save( const GameLibrary &library );

private:
  FileSystem &fs_;
  char path_[96];
  unsigned char *others_; /* the file's records; a known game's is cleared */
  int count_;
};

#endif
