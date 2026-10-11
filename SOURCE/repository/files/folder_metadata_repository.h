#ifndef GMENU_REPOSITORY_FOLDER_METADATA_REPOSITORY_H
#define GMENU_REPOSITORY_FOLDER_METADATA_REPOSITORY_H

#include "../metadata_repository.h"

class FileSystem;

class FolderMetadataRepository : public MetadataRepository {
public:
  /* folder: the game folder's path */
  FolderMetadataRepository( FileSystem &fs, const char *folder );
  bool folderExists();
  Read readMetadata( char *buf, unsigned size );
  int readNameFile( char *buf, unsigned size );
  void listExecutables( ExecutableList &exes );
  bool writeMetadata( const char *text, unsigned len );

private:
  void path( const char *name, char *out ) const;

  FileSystem &fs_;
  const char *folder_;
};

#endif
