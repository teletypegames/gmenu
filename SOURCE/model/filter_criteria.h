/*
   What the game list shows: the search (title, folder name or description contains it)
   and the filters (genre, year, players, favorites only, without METADATA.INI only).
   YEAR "1990s" matches 1990..1999; PLAYERS: a number matches the "a", "a-b" and "a+"
   ranges that contain it.
*/

#ifndef GMENU_MODEL_FILTER_CRITERIA_H
#define GMENU_MODEL_FILTER_CRITERIA_H

#include "limits.h"

struct Game;

enum { MaxSearch = 30 };

struct FilterCriteria {
  char search[32];
  unsigned char searchLen;
  char genre[MaxGenre + 1];
  char year[MaxYear + 1];
  char players[MaxPlayers + 1];
  char favoritesOnly;
  char noMetadataOnly;

  bool filtersActive() const;
  void clearFilters();
  void clearSearch();
  /* false when nothing changed */
  bool addToSearch( char c );
  bool backspace();
  bool matches( const Game &game ) const;
  /* The filter value of a picker field (genre, year, players). */
  char *filterOf( int field );
};

/* "a", "a-b" or "a+" -> a, b ("a+": FFFFh); false if it is not a number. */
bool parseRange( const char **s, unsigned *a, unsigned *b );

#endif
