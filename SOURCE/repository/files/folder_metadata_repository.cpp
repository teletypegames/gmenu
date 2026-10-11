#include <string.h>

#include "folder_metadata_repository.h"
#include "../../lib/core/str.h"
#include "../../lib/platform/file_system.h"
#include "../../model/executable_list.h"
#include "../../model/limits.h"

FolderMetadataRepository::FolderMetadataRepository( FileSystem &fs, const char *folder )
  : fs_( fs ), folder_( folder ) {}

void FolderMetadataRepository::path( const char *name, char *out ) const {
  strcpy( out, folder_ );
  strcat( out, name );
}

bool FolderMetadataRepository::folderExists() {
  DirEntry entry;
  return fs_.find( folder_, AttrFolder, entry ) && entry.isFolder();
}

MetadataRepository::Read FolderMetadataRepository::readMetadata( char *buf, unsigned size ) {
  char file[96];
  path( MetadataFile, file );
  int h = fs_.openRead( file );
  if ( h < 0 ) return Missing;
  int n = fs_.read( h, buf, size - 1 );
  fs_.close( h );
  if ( n < 0 ) return Unreadable;
  if ( n >= (int)size - 1 ) return TooBig;
  buf[n] = 0;
  return Loaded;
}

int FolderMetadataRepository::readNameFile( char *buf, unsigned size ) {
  char file[96];
  path( NameFile, file );
  return readText( fs_, file, buf, size );
}

class Executables : public DirVisitor {
public:
  explicit Executables( ExecutableList &exes ) : exes_( exes ) {}

  bool visit( const DirEntry &entry ) {
    if ( isExecutable( entry.name ) ) exes_.add( entry.name );
    return true;
  }

private:
  ExecutableList &exes_;
};

void FolderMetadataRepository::listExecutables( ExecutableList &exes ) {
  char pattern[96];
  path( "\\*.*", pattern );
  exes.clear();
  Executables visitor( exes );
  fs_.list( pattern, AttrNormal, visitor );
}

bool FolderMetadataRepository::writeMetadata( const char *text, unsigned len ) {
  char temp[96], file[96];
  path( "\\METADATA.$$$", temp );
  path( MetadataFile, file );
  if ( !writeFile( fs_, temp, text, len ) ) {
    fs_.remove( temp );
    return false;
  }
  fs_.remove( file );
  return fs_.rename( temp, file );
}
