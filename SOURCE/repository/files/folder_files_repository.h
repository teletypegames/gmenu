#ifndef GMENU_REPOSITORY_FOLDER_FILES_REPOSITORY_H
#define GMENU_REPOSITORY_FOLDER_FILES_REPOSITORY_H

#include "../game_files_repository.h"

class FileSystem;
struct Locations;

class FolderFilesRepository : public GameFilesRepository {
public:
  FolderFilesRepository( FileSystem &fs, const Locations &where );
  void list( const char *folder, bool all, FileList &files );

private:
  FileSystem &fs_;
  const Locations &where_;
};

#endif
