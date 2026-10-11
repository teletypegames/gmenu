/*
   One game folder's files for GMEDIT: METADATA.INI, NAME.TXT, the executables.
*/

#ifndef GMENU_REPOSITORY_METADATA_REPOSITORY_H
#define GMENU_REPOSITORY_METADATA_REPOSITORY_H

struct ExecutableList;

class MetadataRepository {
public:
  enum Read { Missing, Loaded, Unreadable, TooBig };

  virtual bool folderExists() = 0;
  /* METADATA.INI into buf (terminated), when it is smaller than size. */
  virtual Read readMetadata( char *buf, unsigned size ) = 0;
  /* NAME.TXT (terminated): its length, or -1. */
  virtual int readNameFile( char *buf, unsigned size ) = 0;
  virtual void listExecutables( ExecutableList &exes ) = 0;
  /* Writes METADATA.$$$, then renames it; false when it could not be written. */
  virtual bool writeMetadata( const char *text, unsigned len ) = 0;
};

#endif
