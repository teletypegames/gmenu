#include <string.h>

#include "fav_file_repository.h"
#include "../../lib/platform/file_system.h"
#include "../../model/game_library.h"
#include "../../model/locations.h"

FavFileRepository::FavFileRepository( FileSystem &fs, const Locations &where ) : fs_( fs ) {
  where.progFile( "GMENU.FAV", path_ );
}

static void markFavorite( GameLibrary &library, char *name, int &len ) {
  if ( len == 0 ) return;
  name[len] = 0;
  len = 0;
  int g = library.find( name );
  if ( g >= 0 ) library.at( g ).flags |= GameFavorite;
}

/* The folder names of the file, read a piece at a time (a name split between two pieces
   is joined). */
void FavFileRepository::load( GameLibrary &library ) {
  int h = fs_.openRead( path_ );
  if ( h < 0 ) return;
  char buf[512], name[FolderSize];
  int len = 0, n;
  while ( ( n = fs_.read( h, buf, sizeof( buf ) ) ) > 0 ) {
    for ( int i = 0; i < n; i++ ) {
      if ( (unsigned char)buf[i] <= ' ' ) markFavorite( library, name, len );
      else if ( len < FolderSize - 1 ) name[len++] = buf[i];
    }
  }
  markFavorite( library, name, len );
  fs_.close( h );
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
