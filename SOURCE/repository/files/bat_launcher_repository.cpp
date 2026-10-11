#include "bat_launcher_repository.h"
#include "../../lib/platform/file_system.h"
#include "../../model/locations.h"

BatLauncherRepository::BatLauncherRepository( FileSystem &fs, const Locations &where ) : fs_( fs ), where_( where ) {}

void BatLauncherRepository::writeScript( const char *text, unsigned len ) {
  char path[96];
  where_.progFile( "GMENURUN.BAT", path );
  writeFile( fs_, path, text, len );
}

bool BatLauncherRepository::findEditor( char *out ) {
  DirEntry entry;
  where_.progFile( "GMEDIT.EXE", out );
  return fs_.find( out, AttrNormal, entry );
}
