#include <stdlib.h>
#include <string.h>

#include "sta_file_repository.h"
#include "../../lib/platform/file_system.h"
#include "../../model/game_library.h"
#include "../../model/locations.h"

enum { RecordSize = FolderSize + 4, MaxRecords = 1000 };

static unsigned getWord( const unsigned char *p ) {
  return p[0] | ( p[1] << 8 );
}

static void putWord( unsigned char *p, unsigned w ) {
  p[0] = (unsigned char)w;
  p[1] = (unsigned char)( w >> 8 );
}

StaFileRepository::StaFileRepository( FileSystem &fs, const Locations &where )
  : fs_( fs ), others_( NULL ), count_( 0 ) {
  where.progFile( "GMENU.STA", path_ );
}

StaFileRepository::~StaFileRepository() {
  free( others_ );
}

void StaFileRepository::load( GameLibrary &library ) {
  count_ = 0;
  if ( others_ == NULL ) others_ = (unsigned char *)malloc( RecordSize * MaxRecords );
  if ( others_ == NULL ) return;
  int n = readFile( fs_, path_, others_, RecordSize * MaxRecords );
  if ( n < 0 ) return;
  count_ = n / RecordSize;
  for ( int i = 0; i < count_; i++ ) {
    unsigned char *r = others_ + i * RecordSize;
    char name[FolderSize];
    memcpy( name, r, FolderSize );
    name[FolderSize - 1] = 0;
    int g = library.find( name );
    if ( g < 0 ) continue;
    library.at( g ).plays = getWord( r + FolderSize );
    library.at( g ).last = getWord( r + FolderSize + 2 );
    r[0] = 0;
  }
}

void StaFileRepository::save( const GameLibrary &library ) {
  int h = fs_.create( path_ );
  if ( h < 0 ) return;
  for ( int g = 0; g < library.count(); g++ ) {
    const Game &game = library.at( g );
    unsigned char r[RecordSize];
    if ( game.plays == 0 ) continue;
    memcpy( r, game.folder, FolderSize );
    putWord( r + FolderSize, game.plays );
    putWord( r + FolderSize + 2, game.last );
    fs_.write( h, r, RecordSize );
  }
  for ( int i = 0; i < count_; i++ ) {
    if ( others_[i * RecordSize] ) fs_.write( h, others_ + i * RecordSize, RecordSize );
  }
  fs_.close( h );
}
