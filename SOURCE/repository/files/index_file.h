/*
   GMENU.IDX: the games of the last folder scan with what was read from their
   METADATA.INI / NAME.TXT. The menu starts from it without looking at the folders; a scan
   (the first start, or Ctrl+F) checks it against the files and writes it again.
   'GI', version (2), the game directory (66 bytes), then per game in folder scan order:
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
  /* All its games into the (empty) library; false when there is no file of this game
     directory, or it is broken (the library is cleared then). */
  bool loadAll( GameLibrary &library );
  /* Before the scan: opened when it belongs to this game directory. */
  void open();
  /* true when all the bytes were read; otherwise it is dropped. */
  bool read( void *buf, unsigned n );
  void drop();
  /* After the scan: written again when it changed. */
  void finish( const GameLibrary &library );
  /* The games of the library into the file. */
  void save( const GameLibrary &library );

private:
  enum { BufferSize = 1024 };

  void close();
  bool atEnd();
  bool readGame( GameLibrary &library );
  bool skip( unsigned n );
  void put( int handle, const void *data, unsigned n );
  void flush( int handle );

  FileSystem &fs_;
  const char *path_;
  const char *gameDir_;
  int handle_;
  bool dirty_;
  unsigned char buf_[BufferSize];
  unsigned bufPos_, bufLen_;
};

#endif
