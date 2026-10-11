#ifndef GMENU_REPOSITORY_SEL_FILE_REPOSITORY_H
#define GMENU_REPOSITORY_SEL_FILE_REPOSITORY_H

#include "../session_repository.h"

class FileSystem;
struct Locations;

/* GMENU.SEL: 'GS' + version (1) + folder name (13) + search length (1) + search (32)
   + filters: genre (25), year (11), players (11) + all-files flag (1) + favorites-only
   flag (1) + sort order (1) + without-METADATA.INI-only flag (1) + recently played folder
   names (10 * 13). A file with another magic, version or size is ignored (the format of
   an older build). */
class SelFileRepository : public SessionRepository {
public:
  SelFileRepository( FileSystem &fs, const Locations &where );
  bool load( Session &session );
  void save( const Session &session );

private:
  FileSystem &fs_;
  char path_[96];
};

#endif
