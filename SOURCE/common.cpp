/*
   common.cpp - routines shared by GMENUPRG.EXE and GMEDIT.EXE (see common.h).
*/

#include <bios.h>
#include <direct.h>
#include <dos.h>
#include <fcntl.h>
#include <i86.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "common.h"

char ProgDir[80];
char StartDir[66];
static char FPath[96];

static unsigned short far *const Video = (unsigned short far *)MK_FP( 0xB800, 0 );


/* ---------------------------------------------------------------- strings */

char upcase( char c ) {
  return ( c >= 'a' && c <= 'z' ) ? c - 32 : c;
}

int cmpi( const char *a, const char *b ) {
  for ( ;; ) {
    unsigned char x = upcase( *a++ );
    unsigned char y = upcase( *b++ );
    if ( x != y ) return x > y ? 1 : -1;
    if ( x == 0 ) return 0;
  }
}

int isEol( char c ) {
  return c == 0 || c == 13 || c == 10 || c == 0x1A;
}

const char *skipBlank( const char *s ) {
  while ( *s == ' ' || *s == 9 ) s++;
  return s;
}

const char *trimBack( const char *beg, const char *end ) {
  while ( end != beg && ( end[-1] == ' ' || end[-1] == 9 ) ) end--;
  return end;
}

int isExec( const char *name ) {
  const char *e = strchr( name, '.' );
  if ( e == NULL || strlen( e ) != 4 ) return 0;
  e++;
  return cmpi( e, "COM" ) == 0 || cmpi( e, "EXE" ) == 0 || cmpi( e, "BAT" ) == 0;
}

char *numStr( char *d, unsigned n ) {
  char digits[6];
  int i = 0;
  do {
    digits[i++] = (char)( '0' + n % 10 );
    n /= 10;
  } while ( n );
  while ( i ) *d++ = digits[--i];
  *d = 0;
  return d;
}


/* ---------------------------------------------------------------- folders, files */

/* The folder of the program from its path, up to the last '\'; \GMENU\ when the path
   has none. */
void getProgDir( const char *argv0 ) {
  char cwd[80];
  if ( getcwd( cwd, sizeof( cwd ) ) && strlen( cwd ) >= 3 && strlen( cwd ) - 3 < sizeof( StartDir ) ) {
    strcpy( StartDir, cwd + 3 );
  }
  else StartDir[0] = 0;
  const char *slash = strrchr( argv0, '\\' );
  if ( slash == NULL || slash - argv0 + 1 > (int)sizeof( ProgDir ) - 14 ) {
    strcpy( ProgDir, "\\GMENU\\" );
    return;
  }
  memcpy( ProgDir, argv0, slash - argv0 + 1 );
  ProgDir[slash - argv0 + 1] = 0;
}

const char *progFile( const char *name ) {
  strcpy( FPath, ProgDir );
  strcat( FPath, name );
  return FPath;
}

int readFile( const char *path, void *buf, unsigned size ) {
  int h = open( path, O_RDONLY | O_BINARY );
  if ( h < 0 ) return -1;
  int n = read( h, buf, size );
  close( h );
  return n;
}

int writeFile( const char *path, const void *buf, unsigned len ) {
  int h = open( path, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE );
  if ( h < 0 ) return -1;
  int n = write( h, buf, len );
  close( h );
  return (unsigned)n == len ? 0 : -1;
}

void say( const char *s ) {
  write( 1, s, strlen( s ) );
}

void beep( void ) {
  bdos( 2, 7, 0 );
}


/* ---------------------------------------------------------------- screen */

void setTextMode( void ) {
  union REGS r;
  r.w.ax = 3;
  int86( 0x10, &r, &r );
}

int scrAt( int row, int col ) {
  return row * COLS + col;
}

int putCh( int pos, char c, unsigned char attr ) {
  if ( pos >= 0 && pos < COLS * ROWS ) Video[pos] = ( (unsigned short)attr << 8 ) | (unsigned char)c;
  return pos + 1;
}

int putStr( int pos, const char *s, int max, unsigned char attr ) {
  while ( max-- > 0 && *s ) pos = putCh( pos, *s++, attr );
  return pos;
}

int fillTo( int pos, int end, unsigned char attr ) {
  while ( pos < end ) pos = putCh( pos, ' ', attr );
  return pos;
}

int putAt( int row, int col, const char *s, int max, unsigned char attr ) {
  return putStr( scrAt( row, col ), s, max, attr );
}

int putNum( int pos, unsigned n, unsigned char attr ) {
  char digits[6];
  numStr( digits, n );
  return putStr( pos, digits, 6, attr );
}

int boxTitle( int row, int col, const char *s, int max, unsigned char attr ) {
  int pos = putCh( scrAt( row, col ), ' ', attr );
  pos = putStr( pos, s, max, attr );
  return putCh( pos, ' ', attr );
}

static void boxLine( int row, int left, int right, char l, char fill, char r, unsigned char attr ) {
  int pos = putCh( scrAt( row, left ), l, attr );
  for ( int i = left + 1; i < right; i++ ) pos = putCh( pos, fill, attr );
  putCh( pos, r, attr );
}

void drawBox( int top, int left, int bottom, int right, unsigned char attr ) {
  boxLine( top, left, right, (char)0xC9, (char)0xCD, (char)0xBB, attr );
  for ( int row = top + 1; row < bottom; row++ ) {
    boxLine( row, left, right, (char)0xBA, ' ', (char)0xBA, attr );
  }
  boxLine( bottom, left, right, (char)0xC8, (char)0xCD, (char)0xBC, attr );
}

void hsep( int row, int left, int right, unsigned char attr ) {
  boxLine( row, left, right, (char)0xC7, (char)0xC4, (char)0xB6, attr );
}

int putFooter( int pos, const char *s ) {
  unsigned char attr = 0x70;
  for ( ; *s; s++ ) {
    if ( *s == '~' ) attr ^= 0x04;
    else pos = putCh( pos, *s, attr );
  }
  return pos;
}


/* ---------------------------------------------------------------- UTF-8 */

/* U+00A0..U+00FF: the accented letters CP437 lacks become their base letter. */
static const unsigned char U2cA0[96] = {
  ' ', 0xAD, 0x9B, 0x9C, '?', 0x9D, '|', 0x15, '"', 'c', 0xA6, 0xAE, 0xAA, '-', 'r', '-',
  0xF8, 0xF1, 0xFD, '3', 0x27, 0xE6, 0x14, 0xFA, ',', '1', 0xA7, 0xAF, 0xAC, 0xAB, '?', 0xA8,
  'A', 'A', 'A', 'A', 0x8E, 0x8F, 0x92, 0x80, 'E', 0x90, 'E', 'E', 'I', 'I', 'I', 'I',
  'D', 0xA5, 'O', 'O', 'O', 'O', 0x99, 'x', 'O', 'U', 'U', 'U', 0x9A, 'Y', 'P', 0xE1,
  0x85, 0xA0, 0x83, 'a', 0x84, 0x86, 0x91, 0x87, 0x8A, 0x82, 0x88, 0x89, 0x8D, 0xA1, 0x8C, 0x8B,
  'd', 0xA4, 0x95, 0xA2, 0x93, 'o', 0x94, 0xF6, 'o', 0x97, 0xA3, 0x96, 0x81, 'y', 'p', 0x98
};

/* Other code points. */
static const struct { unsigned cp; unsigned char c; } U2cPairs[] = {
  { 0x150, 0x99 }, { 0x151, 0x94 },     /* O / o with double acute -> umlaut */
  { 0x170, 0x9A }, { 0x171, 0x81 },     /* U / u with double acute -> umlaut */
  { 0x2013, '-' }, { 0x2014, '-' }, { 0x2018, 0x27 }, { 0x2019, 0x27 }, { 0x201A, ',' },
  { 0x201C, '"' }, { 0x201D, '"' }, { 0x201E, '"' }, { 0x2022, 7 }, { 0x2026, '.' },
  { 0, 0 }
};

static unsigned char mapCp( unsigned cp ) {
  if ( cp < 0x80 ) return (unsigned char)cp;
  if ( cp < 0xA0 ) return '?';
  if ( cp < 0x100 ) return U2cA0[cp - 0xA0];
  for ( int i = 0; U2cPairs[i].cp; i++ ) {
    if ( U2cPairs[i].cp == cp ) return U2cPairs[i].c;
  }
  return '?';
}

unsigned utf8To437( char *buf ) {
  unsigned char *s = (unsigned char *)buf;
  unsigned char *d = s;
  unsigned char c;
  while ( ( c = *s++ ) != 0 ) {
    if ( c < 0xC2 || c > 0xF4 || ( s[0] & 0xC0 ) != 0x80 ) {
      *d++ = c;
      continue;
    }
    if ( c < 0xE0 ) {
      *d++ = mapCp( ( ( c & 0x1F ) << 6 ) | ( s[0] & 0x3F ) );
      s++;
    }
    else if ( c < 0xF0 ) {
      if ( ( s[1] & 0xC0 ) != 0x80 ) {
        *d++ = c;
        continue;
      }
      unsigned cp = ( (unsigned)( c & 0x0F ) << 12 ) | ( ( s[0] & 0x3F ) << 6 ) | ( s[1] & 0x3F );
      s += 2;
      if ( cp != 0xFEFF ) *d++ = mapCp( cp );
    }
    else {
      while ( ( *s & 0xC0 ) == 0x80 ) s++;     /* outside the BMP */
      *d++ = '?';
    }
  }
  *d = 0;
  return d - (unsigned char *)buf;
}

const char *skipBom( const char *s ) {
  if ( (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF ) s += 3;
  return s;
}


/* ---------------------------------------------------------------- keyboard */

unsigned keyRead( void ) {
  return _bios_keybrd( _KEYBRD_READ );
}

int keyReady( void ) {
  return _bios_keybrd( _KEYBRD_READY ) != 0;
}

int shiftDown( void ) {
  return ( _bios_keybrd( _KEYBRD_SHIFTSTATUS ) & 3 ) != 0;
}


#ifdef TEST
/* ---------------------------------------------------------------- tests */

static char *TestNext = NULL;

unsigned testKey( void ) {
  if ( TestNext == NULL ) {
    static char keys[4096];
    int n = readFile( progFile( "TESTKEYS.TXT" ), keys, sizeof( keys ) - 1 );
    keys[n < 0 ? 0 : n] = 0;
    TestNext = keys;
  }
  while ( *TestNext == ' ' || *TestNext == 13 || *TestNext == 10 ) TestNext++;
  if ( *TestNext == 0 ) return 0;
  return (unsigned)strtoul( TestNext, &TestNext, 16 );
}

void testScreen( void ) {
  int h = open( progFile( "SCREEN.BIN" ), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE );
  if ( h >= 0 ) {
    write( h, (void far *)Video, COLS * ROWS * 2 );
    close( h );
  }
  exit( 3 );
}
#endif
