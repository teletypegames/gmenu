#include <string.h>

#include "sel_file_repository.h"
#include "../../lib/platform/file_system.h"
#include "../../model/locations.h"
#include "../../model/session.h"
#include "../../model/sort_order.h"

enum {
  SelVersion = 4,
  SelSize = 3 + FolderSize + 1 + 32 + ( MaxGenre + 1 ) + ( MaxYear + 1 ) + ( MaxPlayers + 1 ) + 4 +
            RecentList::Size * FolderSize
};

struct SelField {
  void *var;
  unsigned size;
};

/* The fields of the file, in its order. */
static int fieldsOf( Session &s, SelField *fields ) {
  SelField all[] = { { s.selected, FolderSize },
                     { &s.criteria.searchLen, 1 },
                     { s.criteria.search, 32 },
                     { s.criteria.genre, MaxGenre + 1 },
                     { s.criteria.year, MaxYear + 1 },
                     { s.criteria.players, MaxPlayers + 1 },
                     { &s.showAll, 1 },
                     { &s.criteria.favoritesOnly, 1 },
                     { &s.sortOrder, 1 },
                     { &s.criteria.noMetadataOnly, 1 },
                     { s.recent.folders, RecentList::Size * FolderSize } };
  int n = sizeof( all ) / sizeof( all[0] );
  memcpy( fields, all, sizeof( all ) );
  return n;
}

SelFileRepository::SelFileRepository( FileSystem &fs, const Locations &where ) : fs_( fs ) {
  where.progFile( "GMENU.SEL", path_ );
}

static void sanitize( Session &s ) {
  if ( s.criteria.searchLen > MaxSearch ) s.criteria.searchLen = 0;
  s.selected[FolderSize - 1] = 0;
  s.criteria.genre[MaxGenre] = 0;
  s.criteria.year[MaxYear] = 0;
  s.criteria.players[MaxPlayers] = 0;
  s.showAll &= 1;
  s.criteria.favoritesOnly &= 1;
  s.criteria.noMetadataOnly &= 1;
  for ( int r = 0; r < RecentList::Size; r++ ) s.recent.folders[r][FolderSize - 1] = 0;
  if ( (unsigned char)s.sortOrder >= SortOrderCount ) s.sortOrder = 0;
}

bool SelFileRepository::load( Session &session ) {
  unsigned char buf[SelSize + 1];
  int n = readFile( fs_, path_, buf, sizeof( buf ) );
  if ( n != SelSize || buf[0] != 'G' || buf[1] != 'S' || buf[2] != SelVersion ) return false;
  SelField fields[12];
  int count = fieldsOf( session, fields );
  const unsigned char *p = buf + 3;
  for ( int i = 0; i < count; i++ ) {
    memcpy( fields[i].var, p, fields[i].size );
    p += fields[i].size;
  }
  sanitize( session );
  return true;
}

void SelFileRepository::save( const Session &session ) {
  unsigned char buf[SelSize];
  SelField fields[12];
  int count = fieldsOf( (Session &)session, fields );
  unsigned char *p = buf + 3;
  buf[0] = 'G';
  buf[1] = 'S';
  buf[2] = SelVersion;
  for ( int i = 0; i < count; i++ ) {
    memcpy( p, fields[i].var, fields[i].size );
    p += fields[i].size;
  }
  writeFile( fs_, path_, buf, SelSize );
}
