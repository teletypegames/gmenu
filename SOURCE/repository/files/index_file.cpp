#include <string.h>

#include "index_file.h"
#include "../../lib/platform/file_system.h"
#include "../../model/game_library.h"

static const unsigned char Version = 1;
static const unsigned DirSize = 66;

IndexFile::IndexFile( FileSystem &fs, const char *path, const char *gameDir )
  : fs_( fs ), path_( path ), gameDir_( gameDir ), handle_( -1 ), dirty_( true ) {}

void IndexFile::close() {
  if ( handle_ >= 0 ) fs_.close( handle_ );
  handle_ = -1;
}

void IndexFile::drop() {
  dirty_ = true;
  close();
}

bool IndexFile::read( void *buf, unsigned n ) {
  if ( handle_ < 0 ) return false;
  if ( (unsigned)fs_.read( handle_, buf, n ) == n ) return true;
  drop();
  return false;
}

void IndexFile::open() {
  unsigned char header[3 + DirSize];
  dirty_ = true;
  handle_ = fs_.openRead( path_ );
  if ( handle_ < 0 ) return;
  dirty_ = false;
  if ( !read( header, sizeof( header ) ) ) return;
  if ( header[0] != 'G' || header[1] != 'I' || header[2] != Version || memcmp( header + 3, gameDir_, DirSize ) ) drop();
}

void IndexFile::finish( const GameLibrary &library ) {
  if ( handle_ >= 0 ) {
    unsigned char more;
    if ( fs_.read( handle_, &more, 1 ) != 0 ) dirty_ = true;
    close();
  }
  if ( dirty_ ) write( library );
}

static unsigned recordLength( const Game &game ) {
  unsigned len = 0;
  if ( game.record ) {
    for ( int f = 0; f < FieldCount; f++ ) len += strlen( game.record + len ) + 1;
  }
  return len;
}

void IndexFile::write( const GameLibrary &library ) {
  int h = fs_.create( path_ );
  if ( h < 0 ) return;
  unsigned char header[3 + DirSize];
  header[0] = 'G';
  header[1] = 'I';
  header[2] = Version;
  memcpy( header + 3, gameDir_, DirSize );
  fs_.write( h, header, sizeof( header ) );
  for ( int g = 0; g < library.count(); g++ ) {
    const Game &game = library.at( g );
    unsigned char out[HeaderSize + MaxName + 2];
    unsigned titleLen = strlen( game.title );
    unsigned recLen = recordLength( game );
    memcpy( out, game.folder, FolderSize );
    memcpy( out + FolderSize, game.stamp, StampSize );
    out[FolderSize + StampSize] = ( game.flags & GameTitleNotFromMetadata ) ? 1 : 0;
    out[HeaderSize - 1] = (unsigned char)titleLen;
    memcpy( out + HeaderSize, game.title, titleLen );
    out[HeaderSize + titleLen] = (unsigned char)recLen;
    out[HeaderSize + titleLen + 1] = (unsigned char)( recLen >> 8 );
    fs_.write( h, out, HeaderSize + titleLen + 2 );
    if ( recLen ) fs_.write( h, game.record, recLen );
  }
  fs_.close( h );
}
