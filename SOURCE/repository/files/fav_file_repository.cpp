#include <stdlib.h>
#include <string.h>

#include "fav_file_repository.h"
#include "../../lib/platform/file_system.h"
#include "../../model/game_library.h"
#include "../../model/locations.h"

static const unsigned FavSize = GameLibrary::Capacity * FolderSize;

FavFileRepository::FavFileRepository( FileSystem &fs, const Locations &where ) : fs_( fs ) {
  where.progFile( "GMENU.FAV", path_ );
}

void FavFileRepository::load( GameLibrary &library ) {
  char *buf = (char *)malloc( FavSize );
  if ( buf == NULL ) return;
  int n = readText( fs_, path_, buf, FavSize );
  for ( char *s = buf; n > 0 && *s; ) {
    if ( (unsigned char)*s <= ' ' ) {
      s++;
      continue;
    }
    char *name = s;
    while ( (unsigned char)*s > ' ' ) s++;
    char sep = *s;
    *s = 0;
    int g = library.find( name );
    if ( g >= 0 ) library.at( g ).flags |= GameFavorite;
    if ( sep == 0 ) break;
    s++;
  }
  free( buf );
}

void FavFileRepository::save( const GameLibrary &library ) {
  int h = fs_.create( path_ );
  if ( h < 0 ) return;
  for ( int i = 0; i < library.count(); i++ ) {
    const Game &game = library.at( library.ordered( i ) );
    if ( !game.isFavorite() ) continue;
    fs_.write( h, game.folder, strlen( game.folder ) );
    fs_.write( h, "\r\n", 2 );
  }
  fs_.close( h );
}
