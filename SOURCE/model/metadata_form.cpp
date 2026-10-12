#include <string.h>

#include "executable_list.h"
#include "metadata_form.h"
#include "../lib/core/ini.h"
#include "../lib/core/str.h"
#include "../lib/core/text.h"

static const char *const Keys[FormFieldCount] = { "NAME",        "GENRE", "YEAR", "PLAYERS",
                                                  "DESCRIPTION", "RUN",   "ARGS", "SETUP" };
static const int Limits[FormFieldCount] = { MaxName, MaxGenre,   MaxYear,    MaxPlayers,
                                            MaxDesc, MaxCommand, MaxCommand, MaxCommand };

static int offsetOf( int field ) {
  int off = 0;
  for ( int f = 0; f < field; f++ ) off += Limits[f] + 1;
  return off;
}

static int fieldOf( const IniLine &line ) {
  for ( int f = 0; f < FormFieldCount; f++ ) {
    if ( line.keyIs( Keys[f] ) ) return f;
  }
  return -1;
}

MetadataForm::MetadataForm() : raw_( NULL ) {
  memset( values_, 0, sizeof( values_ ) );
  memset( original_, 0, sizeof( original_ ) );
}

char *MetadataForm::value( int field ) {
  return values_ + offsetOf( field );
}

int MetadataForm::maxLength( int field ) const {
  return Limits[field];
}

bool MetadataForm::isExisting() const {
  return raw_ != NULL;
}

void MetadataForm::keepAsOriginal() {
  memcpy( original_, values_, sizeof( values_ ) );
}

bool MetadataForm::isChanged() const {
  for ( int f = 0; f < FormFieldCount; f++ ) {
    if ( strcmp( values_ + offsetOf( f ), original_ + offsetOf( f ) ) ) return true;
  }
  return false;
}

/* A value (in CP437, clipped) into the field; a further DESCRIPTION line is appended
   after \n (as the menu reads it: not after an empty start). */
void MetadataForm::loadValue( int field, const char *v, int len ) {
  if ( len > (int)sizeof( text_ ) - 1 ) len = sizeof( text_ ) - 1;
  memcpy( text_, v, len );
  text_[len] = 0;
  utf8To437( text_ );
  char *d = value( field );
  char *end = d + Limits[field];
  if ( field == FormDesc && *d ) {
    d += strlen( d );
    if ( d + 2 > end ) {
      *d = 0;
      return;
    }
    *d++ = '\\';
    *d++ = 'n';
  }
  for ( const char *s = text_; d < end && *s; ) *d++ = *s++;
  *d = 0;
}

void MetadataForm::load( const char *raw ) {
  raw_ = raw;
  IniScanner scanner( raw );
  IniLine line;
  while ( scanner.next( line ) ) {
    int f = fieldOf( line );
    if ( f >= 0 ) loadValue( f, line.value, line.valueLength );
  }
}

static bool isSetup( const char *name ) {
  return strncmp( name, "SETUP", 5 ) == 0 || strncmp( name, "INSTALL", 7 ) == 0;
}

void MetadataForm::prefill( const char *nameText, int len, const char *folder, const ExecutableList &exes ) {
  char *name = value( FormName );
  int n = 0;
  while ( nameText && len-- > 0 ) {
    char c = *nameText++;
    if ( isEol( c ) ) break;
    if ( c == '\t' ) c = ' ';
    if ( c == ' ' && n == 0 ) continue;
    if ( n >= MaxName ) break;
    name[n++] = c;
  }
  while ( n && name[n - 1] == ' ' ) n--;
  name[n] = 0;
  if ( !*name ) {
    const char *s = folder;
    for ( const char *p = folder; *p; p++ ) {
      if ( *p == '\\' || *p == ':' ) s = p + 1;
    }
    strcpy( name, s );
  }
  int plays = 0;
  const char *run = NULL;
  for ( int i = 0; i < exes.count; i++ ) {
    if ( isSetup( exes.names[i] ) ) {
      if ( !*value( FormSetup ) ) strcpy( value( FormSetup ), exes.names[i] );
    }
    else {
      plays++;
      run = exes.names[i];
    }
  }
  if ( plays == 1 ) strcpy( value( FormRun ), run );
}

static bool isBlank( const char *s ) {
  while ( *s == ' ' ) s++;
  return *s == 0;
}

/* "KEY=value" line in UTF-8 (none when the value is blank). */
static char *putField( char *d, int field, const char *v ) {
  if ( isBlank( v ) ) return d;
  strcpy( d, Keys[field] );
  d += strlen( d );
  *d++ = '=';
  d = cp437ToUtf8( d, v );
  *d++ = '\r';
  *d++ = '\n';
  return d;
}

enum { Unchanged, Changed, Written };

void MetadataForm::detectChanges( char *state ) const {
  for ( int f = 0; f < FormFieldCount; f++ ) {
    state[f] = strcmp( values_ + offsetOf( f ), original_ + offsetOf( f ) ) ? Changed : Unchanged;
  }
}

char *MetadataForm::writeNewHeader( char *d ) const {
  strcpy( d, "[GAME]\r\n" );
  return d + strlen( d );
}

char *MetadataForm::mergeWithRaw( char *d, char *state ) const {
  IniScanner scanner( raw_ );
  IniLine line;
  memcpy( d, raw_, scanner.body() - raw_ );
  d += scanner.body() - raw_;
  while ( scanner.next( line ) ) {
    int f = fieldOf( line );
    if ( f >= 0 && state[f] != Unchanged ) {
      if ( state[f] == Changed ) {
        state[f] = Written;
        d = putField( d, f, values_ + offsetOf( f ) );
      }
      continue;
    }
    memcpy( d, line.begin, line.end - line.begin );
    d += line.end - line.begin;
    *d++ = '\r';
    *d++ = '\n';
  }
  return d;
}

char *MetadataForm::appendRemainingFields( char *d, const char *state ) const {
  for ( int f = 0; f < FormFieldCount; f++ ) {
    if ( state[f] == Changed ) d = putField( d, f, values_ + offsetOf( f ) );
  }
  return d;
}

unsigned MetadataForm::write( char *out ) const {
  char state[FormFieldCount];
  detectChanges( state );
  char *d = out;
  if ( raw_ == NULL ) d = writeNewHeader( d );
  else d = mergeWithRaw( d, state );
  d = appendRemainingFields( d, state );
  return d - out;
}
