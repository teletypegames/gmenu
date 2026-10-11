/*
   GMENU.IDX: what was read from the games' METADATA.INI / NAME.TXT, kept between runs.
   'GI', version, the game directory (66 bytes), then per game in folder scan order:
   folder (13), stamps (16: METADATA.INI, NAME.TXT: time, date, size), source (1: 1 = the
   title came from NAME.TXT / the folder name), title length (1), title, record length
   (2), record. A failed read drops it: it is written again after the scan.
*/

#ifndef GMENU_REPOSITORY_INDEX_FILE_H
#define GMENU_REPOSITORY_INDEX_FILE_H

class FileSystem;
class GameLibrary;

class IndexFile {
public:
  enum { HeaderSize = 13 + 16 + 2 };

  IndexFile( FileSystem &fs, const char *path, const char *gameDir );
  /* Before the scan: opened when it belongs to this game directory. */
  void open();
  /* true when all the bytes were read; otherwise it is dropped. */
  bool read( void *buf, unsigned n );
  void drop();
  /* After the scan: written again when it changed. */
  void finish( const GameLibrary &library );

private:
  void close();
  void write( const GameLibrary &library );

  FileSystem &fs_;
  const char *path_;
  const char *gameDir_;
  int handle_;
  bool dirty_;
};

#endif
