#ifndef GMENU_REPOSITORY_BAT_LAUNCHER_REPOSITORY_H
#define GMENU_REPOSITORY_BAT_LAUNCHER_REPOSITORY_H

#include "../launcher_repository.h"

class FileSystem;
struct Locations;

class BatLauncherRepository : public LauncherRepository {
public:
  BatLauncherRepository( FileSystem &fs, const Locations &where );
  void writeScript( const char *text, unsigned len );
  bool findEditor( char *out );

private:
  FileSystem &fs_;
  const Locations &where_;
};

#endif
