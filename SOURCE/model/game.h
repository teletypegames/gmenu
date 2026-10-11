/*
   A game: a folder of the game directory. Its title and record come from its
   METADATA.INI (NAME, GENRE, YEAR, PLAYERS, DESCRIPTION, RUN, SETUP, ARGS), without one
   from NAME.TXT or the folder name. RUN / SETUP are command lines (file [arguments]),
   ARGS are arguments added to RUN.
*/

#ifndef GMENU_MODEL_GAME_H
#define GMENU_MODEL_GAME_H

#include "limits.h"

enum GameField { FieldGenre, FieldYear, FieldPlayers, FieldDesc, FieldRun, FieldSetup, FieldArgs, FieldCount };

enum GameFlag { GameFavorite = 1, GameTitleNotFromMetadata = 2, GameNoMetadata = 4 };

enum { StampSize = 16 };

struct Game {
  char folder[FolderSize];
  char title[MaxName + 1];
  char *record;                   /* genre,0,year,0,players,0,desc,0,run,0,setup,0,args,0 or NULL */
  unsigned char stamp[StampSize]; /* METADATA.INI, NAME.TXT: time, date, size */
  unsigned char flags;
  unsigned plays; /* starts, and the date of the last one (DOS format) */
  unsigned last;

  /* A field of the record; empty when the game has none. */
  const char *field( int f ) const;
  bool isFavorite() const;
  /* "5x, last 2026-10-09" or "never" */
  void playedText( char *out ) const;
};

/* Title, then folder. */
int compareTitles( const Game &a, const Game &b );

#endif
