#include <string.h>

#include "folder_game_repository.h"
#include "../../lib/core/str.h"
#include "../../lib/core/text.h"
#include "../../lib/platform/file_system.h"
#include "../../model/game_library.h"
#include "../../model/locations.h"

class GameFolders : public DirVisitor {
public:
  GameFolders( FolderGameRepository &repository, GameLibrary &library )
    : repository_( repository ), library_( library ) {}

  bool visit( const DirEntry &entry ) {
    if ( !entry.isFolder() || entry.name[0] == '.' ) return true;
    if ( library_.isFull() ) return false;
    repository_.readGame( library_.at( library_.add( entry.name ) ) );
    return true;
  }

private:
  FolderGameRepository &repository_;
  GameLibrary &library_;
};

FolderGameRepository::FolderGameRepository( FileSystem &fs, const Locations &where )
  : fs_( fs ), where_( where ), library_( NULL ), index_( fs, indexPath_, where.gameDir ), titleFromNameFile_( false ) {
  where.progFile( "GMENU.IDX", indexPath_ );
}

void FolderGameRepository::load( GameLibrary &library ) {
  library_ = &library;
  char pattern[96];
  strcpy( pattern, where_.gameDir );
  strcat( pattern, "\\*.*" );
  index_.open();
  GameFolders folders( *this, library );
  fs_.list( pattern, AttrFolder, folders );
  index_.finish( library );
}

void FolderGameRepository::fileStamp( const Game &game, const char *name, unsigned char *out ) {
  char path[128];
  DirEntry entry;
  where_.gameFile( game.folder, name, path );
  if ( !fs_.find( path, AttrHidden | AttrSystem, entry ) ) {
    memset( out, 0, 8 );
    return;
  }
  out[0] = (unsigned char)entry.time;
  out[1] = (unsigned char)( entry.time >> 8 );
  out[2] = (unsigned char)entry.date;
  out[3] = (unsigned char)( entry.date >> 8 );
  for ( int i = 0; i < 4; i++ ) out[4 + i] = (unsigned char)( entry.size >> ( 8 * i ) );
}

int FolderGameRepository::readGameFile( const Game &game, const char *name ) {
  char path[128];
  where_.gameFile( game.folder, name, path );
  if ( readText( fs_, path, text_, sizeof( text_ ) ) < 0 ) return -1;
  return utf8To437( text_ );
}

void FolderGameRepository::readGame( Game &game ) {
  titleFromNameFile_ = false;
  strcpy( game.title, game.folder );
  fileStamp( game, MetadataFile, stamp_ );
  memset( stamp_ + 8, 0, 8 );
  if ( !fromIndex( game ) ) {
    game.record = NULL;
    titleFromNameFile_ = false;
    strcpy( game.title, game.folder );
    fromFiles( game );
  }
  keepStamp( game );
}

bool FolderGameRepository::fromIndex( Game &game ) {
  unsigned char header[IndexFile::HeaderSize];
  unsigned len = 0;
  if ( !index_.read( header, sizeof( header ) ) ) return false;
  bool same = cmpi( (char *)header, game.folder ) == 0 && memcmp( header + FolderSize, stamp_, 8 ) == 0;
  if ( same ) {
    titleFromNameFile_ = header[FolderSize + StampSize] != 0;
    if ( titleFromNameFile_ ) {
      fileStamp( game, NameFile, stamp_ + 8 );
      same = memcmp( header + FolderSize + 8, stamp_ + 8, 8 ) == 0;
    }
  }
  len = header[IndexFile::HeaderSize - 1];
  if ( !same || len > MaxName ) {
    index_.drop();
    return false;
  }
  if ( !index_.read( game.title, len ) ) return false;
  game.title[len] = 0;
  unsigned char size[2];
  if ( !index_.read( size, 2 ) ) return false;
  len = size[0] | ( size[1] << 8 );
  if ( len == 0 ) return true;
  char *record = library_->allocRecord( len );
  if ( record == NULL ) {
    index_.drop();
    return false;
  }
  if ( !index_.read( record, len ) ) return false;
  game.record = record;
  return true;
}

void FolderGameRepository::fromFiles( Game &game ) {
  if ( readGameFile( game, MetadataFile ) >= 0 ) {
    metadata_.parse( text_ );
    metadata_.storeRecord( game, *library_ );
    if ( metadata_.hasName() ) {
      metadata_.applyTitle( game );
      return;
    }
  }
  titleFromNameFile_ = true;
  fileStamp( game, NameFile, stamp_ + 8 );
  int n = readGameFile( game, NameFile );
  if ( n < 0 ) return;
  const char *s = skipBom( text_ );
  setTitle( game, s, n - ( s - text_ ) );
}

void FolderGameRepository::keepStamp( Game &game ) {
  memcpy( game.stamp, stamp_, StampSize );
  if ( stamp_[2] == 0 && stamp_[3] == 0 ) game.flags |= GameNoMetadata;
  if ( titleFromNameFile_ ) game.flags |= GameTitleNotFromMetadata;
}
