#include <string.h>

#include "index_file.h"
#include "../../lib/platform/file_system.h"
#include "../../model/game_library.h"

static const unsigned char Version = 2;
static const unsigned DirSize = 66;

IndexFile::IndexFile( FileSystem &fs, const char *path, const char *gameDir )
  : fs_( fs ), path_( path ), gameDir_( gameDir ), handle_( -1 ), dirty_( true ), bufPos_( 0 ), bufLen_( 0 ) {}

void IndexFile::close() {
  if ( handle_ >= 0 ) fs_.close( handle_ );
  handle_ = -1;
  bufPos_ = bufLen_ = 0;
}

void IndexFile::drop() {
  dirty_ = true;
  close();
}

bool IndexFile::read( void *buf, unsigned n ) {
  unsigned char *d = (unsigned char *)buf;
  while ( n ) {
    if ( handle_ < 0 ) return false;
    if ( bufPos_ == bufLen_ ) {
      int got = fs_.read( handle_, buf_, BufferSize );
      if ( got <= 0 ) {
        drop();
        return false;
      }
      bufPos_ = 0;
      bufLen_ = got;
    }
    unsigned take = bufLen_ - bufPos_ < n ? bufLen_ - bufPos_ : n;
    memcpy( d, buf_ + bufPos_, take );
    bufPos_ += take;
    d += take;
    n -= take;
  }
  return true;
}

bool IndexFile::skip( unsigned n ) {
  unsigned char waste[64];
  while ( n ) {
    unsigned take = n < sizeof( waste ) ? n : sizeof( waste );
    if ( !read( waste, take ) ) return false;
    n -= take;
  }
  return true;
}

/* No more bytes (the file stays open). */
bool IndexFile::atEnd() {
  if ( handle_ < 0 ) return true;
  if ( bufPos_ < bufLen_ ) return false;
  int got = fs_.read( handle_, buf_, BufferSize );
  bufPos_ = 0;
  bufLen_ = got > 0 ? got : 0;
  return bufLen_ == 0;
}

void IndexFile::open() {
  unsigned char header[3 + DirSize];
  close();
  dirty_ = true;
  handle_ = fs_.openRead( path_ );
  if ( handle_ < 0 ) return;
  dirty_ = false;
  if ( !read( header, sizeof( header ) ) ) return;
  if ( header[0] != 'G' || header[1] != 'I' || header[2] != Version || memcmp( header + 3, gameDir_, DirSize ) ) drop();
}

/* The next game of the file into the library; false when it is broken. */
bool IndexFile::readGame( GameLibrary &library ) {
  unsigned char header[HeaderSize];
  if ( !read( header, sizeof( header ) ) ) return false;
  header[FolderSize - 1] = 0;
  unsigned titleLen = header[HeaderSize - 1];
  if ( titleLen > MaxName ) return false;
  int g = library.add( (const char *)header );
  if ( g < 0 ) return false;
  Game &game = library.at( g );
  memcpy( game.stamp, header + FolderSize, StampSize );
  if ( header[FolderSize + StampSize] ) game.flags |= GameTitleNotFromMetadata;
  if ( game.stamp[2] == 0 && game.stamp[3] == 0 ) game.flags |= GameNoMetadata;
  if ( !read( game.title, titleLen ) ) return false;
  game.title[titleLen] = 0;
  unsigned char size[2];
  if ( !read( size, 2 ) ) return false;
  unsigned len = size[0] | ( size[1] << 8 );
  if ( len == 0 ) return true;
  game.record = library.allocRecord( len );
  return game.record ? read( game.record, len ) : skip( len );
}

bool IndexFile::loadAll( GameLibrary &library ) {
  library.clear();
  open();
  while ( handle_ >= 0 && !atEnd() ) {
    if ( !readGame( library ) ) break;
  }
  bool ok = handle_ >= 0 && library.count() > 0;
  close();
  if ( !ok ) library.clear();
  return ok;
}

void IndexFile::finish( const GameLibrary &library ) {
  if ( handle_ >= 0 && !atEnd() ) dirty_ = true;
  close();
  if ( dirty_ ) save( library );
}

static unsigned recordLength( const Game &game ) {
  unsigned len = 0;
  if ( game.record ) {
    for ( int f = 0; f < FieldCount; f++ ) len += strlen( game.record + len ) + 1;
  }
  return len;
}

void IndexFile::put( int handle, const void *data, unsigned n ) {
  const unsigned char *s = (const unsigned char *)data;
  while ( n ) {
    if ( bufLen_ == BufferSize ) flush( handle );
    unsigned take = BufferSize - bufLen_ < n ? BufferSize - bufLen_ : n;
    memcpy( buf_ + bufLen_, s, take );
    bufLen_ += take;
    s += take;
    n -= take;
  }
}

void IndexFile::flush( int handle ) {
  if ( bufLen_ ) fs_.write( handle, buf_, bufLen_ );
  bufLen_ = 0;
}

void IndexFile::save( const GameLibrary &library ) {
  close();
  int h = fs_.create( path_ );
  if ( h < 0 ) return;
  unsigned char header[3 + DirSize];
  header[0] = 'G';
  header[1] = 'I';
  header[2] = Version;
  memcpy( header + 3, gameDir_, DirSize );
  put( h, header, sizeof( header ) );
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
    put( h, out, HeaderSize + titleLen + 2 );
    if ( recLen ) put( h, game.record, recLen );
  }
  flush( h );
  fs_.close( h );
  dirty_ = false;
}
