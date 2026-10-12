/*
   The values of a METADATA.INI as the menu reads them (the text already in code page
   437). Every DESCRIPTION= line starts a new line of the description; in its value "\n"
   is a line break and "\\" a backslash.
*/

#ifndef GMENU_MODEL_METADATA_H
#define GMENU_MODEL_METADATA_H

#include "limits.h"

struct Game;
class GameLibrary;

class Metadata {
public:
  void parse( const char *text );
  bool hasName() const;
  /* The title from NAME. */
  void applyTitle( Game &game ) const;
  /* GENRE..ARGS as the game's new record from the library's pool (none when they are all
     empty or there is no memory). */
  void storeRecord( Game &game, GameLibrary &library );

private:
  enum { KeyCount = 8, KeyName = 0, KeyDesc = 4 };

  void appendDescription( const char *v, int len );

  const char *values_[KeyCount];
  int lengths_[KeyCount];
  char desc_[MaxDesc + 2];
};

/* The first line of the text becomes the title (at most 50 characters, tabs as spaces,
   without the spaces around it); an empty one leaves the folder name. */
void setTitle( Game &game, const char *s, int len );

#endif
