#include "folder_files_repository.h"
#include "../../lib/core/str.h"
#include "../../lib/platform/file_system.h"
#include "../../model/file_list.h"
#include "../../model/locations.h"

class FolderFiles : public DirVisitor {
public:
  FolderFiles( bool all, FileList &files ) : all_( all ), files_( files ) {}

  bool visit( const DirEntry &entry ) {
    if ( !all_ && !isExecutable( entry.name ) ) return true;
    return files_.add( entry.name );
  }

private:
  bool all_;
  FileList &files_;
};

FolderFilesRepository::FolderFilesRepository( FileSystem &fs, const Locations &where ) : fs_( fs ), where_( where ) {}

void FolderFilesRepository::list( const char *folder, bool all, FileList &files ) {
  char pattern[128];
  where_.gameFile( folder, "\\*.*", pattern );
  files.clear();
  FolderFiles visitor( all, files );
  fs_.list( pattern, AttrNormal, visitor );
}
