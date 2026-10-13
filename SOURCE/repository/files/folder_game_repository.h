/*
   The games as the folders of the game directory. The menu starts with the games of
   GMENU.IDX. A scan lists the folders, and a game's title and record come from GMENU.IDX
   when the date, time and size of its METADATA.INI (and of NAME.TXT, if the title came
   from there) did not change; otherwise from the files.
*/

#ifndef GMENU_REPOSITORY_FOLDER_GAME_REPOSITORY_H
#define GMENU_REPOSITORY_FOLDER_GAME_REPOSITORY_H

#include "../game_repository.h"
#include "index_file.h"
#include "../../model/game.h"
#include "../../model/metadata.h"

class FileSystem;
class Progress;
struct Game;
struct Locations;

class FolderGameRepository : public GameRepository {
public:
  FolderGameRepository( FileSystem &fs, const Locations &where );
  void load( GameLibrary &library, Progress &progress );
  void scan( GameLibrary &library, Progress &progress );
  bool recheck( GameLibrary &library, int game );
  /* One more game of the scan. */
  void readGame( Game &game );

private:
  bool fromIndex( Game &game );
  void fromFiles( Game &game );
  int readGameFile( const Game &game, const char *name );
  void fileStamp( const Game &game, const char *name, unsigned char *out );
  void keepStamp( Game &game );

  FileSystem &fs_;
  const Locations &where_;
  GameLibrary *library_;
  char indexPath_[96];
  IndexFile index_;
  unsigned char stamp_[StampSize];
  bool titleFromNameFile_;
  Metadata metadata_;
  char text_[4096];
};

#endif
