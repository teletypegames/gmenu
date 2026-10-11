#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "metadata.h"
#include "../lib/core/ini.h"
#include "../lib/core/str.h"

static const char *const Keys[] = { "NAME", "GENRE", "YEAR", "PLAYERS", "DESCRIPTION", "RUN", "SETUP", "ARGS" };
static const int Limits[] = { MaxName, MaxGenre, MaxYear, MaxPlayers, MaxDesc, MaxCommand, MaxCommand, MaxCommand };

void Metadata::appendDescription( const char *v, int len ) {
  char *d = desc_ + lengths_[KeyDesc];
  char *lim = desc_ + MaxDesc;
  values_[KeyDesc] = desc_;
  if ( lengths_[KeyDesc] ) {
    if ( d >= lim ) return;
    *d++ = '\n';
  }
  while ( len-- > 0 ) {
    char c = *v++;
    if ( c == '\\' && len > 0 ) {
      if ( *v == '\\' ) v++, len--;
      else if ( ( *v | 0x20 ) == 'n' ) c = '\n', v++, len--;
    }
    if ( d >= lim ) break;
    *d++ = c;
  }
  lengths_[KeyDesc] = d - desc_;
}

void Metadata::parse( const char *text ) {
  memset( lengths_, 0, sizeof( lengths_ ) );
  IniScanner scanner( text );
  IniLine line;
  while ( scanner.next( line ) ) {
    int k;
    for ( k = 0; k < KeyCount && !line.keyIs( Keys[k] ); k++ );
    if ( k == KeyCount ) continue;
    if ( k == KeyDesc ) appendDescription( line.value, line.valueLength );
    else {
      values_[k] = line.value;
      lengths_[k] = line.valueLength;
    }
  }
  for ( int k = 0; k < KeyCount; k++ ) {
    if ( lengths_[k] > Limits[k] ) lengths_[k] = Limits[k];
  }
}

bool Metadata::hasName() const {
  return lengths_[KeyName] != 0;
}

void Metadata::applyTitle( Game &game ) const {
  setTitle( game, values_[KeyName], lengths_[KeyName] );
}

void Metadata::storeRecord( Game &game ) {
  unsigned sum = 0;
  for ( int k = 1; k < KeyCount; k++ ) sum += lengths_[k];
  if ( sum == 0 ) return;
  char *r = (char *)malloc( sum + KeyCount - 1 );
  if ( r == NULL ) return;
  game.record = r;
  for ( int k = 1; k < KeyCount; k++ ) {
    const char *s = values_[k];
    for ( int n = lengths_[k]; n; n-- ) {
      char c = *s++;
      *r++ = ( c == '\t' ) ? ' ' : c;
    }
    *r++ = 0;
  }
}

void setTitle( Game &game, const char *s, int len ) {
  char *d = game.title;
  int n = 0;
  while ( len-- > 0 ) {
    char c = *s++;
    if ( isEol( c ) ) break;
    if ( c == '\t' ) c = ' ';
    if ( c == ' ' && n == 0 ) continue;
    if ( n >= MaxName ) break;
    d[n++] = c;
  }
  while ( n && d[n - 1] == ' ' ) n--;
  if ( n == 0 ) strcpy( game.title, game.folder );
  else d[n] = 0;
}
