/*
   Where things are: the menu's own folder (its files: GMENURUN.BAT, GMENU.SEL / .FAV /
   .STA / .IDX), the folder it was started from, and the game directory.
*/

#ifndef GMENU_MODEL_LOCATIONS_H
#define GMENU_MODEL_LOCATIONS_H

struct Locations {
  char progDir[80];          /* with a trailing '\' */
  char startDir[66];         /* without the drive and the leading '\' */
  char gameDir[66];          /* [X:]\path, without a trailing '\' */
  char gameDrive, homeDrive; /* the drive of the games, of the start */

  /* progDir from the program's path (\GMENU\ when it has no folder). */
  void setProgDir( const char *argv0 );
  /* The game directory as given ("D:\GAMES", "\COLLECT", "COLLECT"), upper case. */
  void setGameDir( const char *dir );
  /* gameDir without the drive, for cd. */
  const char *gamePath() const;
  /* progDir + name into out. */
  void progFile( const char *name, char *out ) const;
  /* gameDir\folder + name ("\X.Y") into out. */
  void gameFile( const char *folder, const char *name, char *out ) const;
};

#endif
