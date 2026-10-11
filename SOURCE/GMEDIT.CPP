/*
   GMEDIT.EXE - creates or edits the METADATA.INI of a game folder (DOS / DOSBox)
   Build: build.sh (Open Watcom)
   Launch: GMEDIT.EXE [/B] <game folder>    e.g. GMEDIT C:\COLLECT\DOOM
           GMENUPRG.EXE starts it through GMENURUN.BAT (Ctrl+E) with /B: an error message
           then waits for a key, otherwise the menu would clear it at once
   Exit code: 0 = METADATA.INI written, 1 = cancelled or nothing changed, 2 = error

   A new file's form starts with what the folder tells: NAME from NAME.TXT (or the folder
   name), RUN if the folder has one executable besides the setup programs, SETUP from the
   first SETUP* / INSTALL* executable. An existing file is changed line by line: only the
   lines of the changed values are rewritten (a value's first line; its further lines, e.g.
   more DESCRIPTION= lines, are dropped), the other lines (comments, other keys, the
   values as they were, a BOM) are kept. A new value is written in UTF-8; an emptied value
   loses its line. The file is written as METADATA.$$$, then renamed.
*/

#include <dos.h>
#include <fcntl.h>
#include <i86.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#define NFLD    8               /* form fields */
#define IBUFSZ  4096            /* NAME.TXT read buffer, a value converted to CP437 */
#define RAWSZ   8192            /* largest METADATA.INI that can be edited */
#define MAXX    100             /* executables kept */
#define XCOLS   5               /* executables shown: columns x rows, column width */
#define XROWS   5
#define XW      14
#define VCOL    16              /* first column of the values */
#define F_DESC  4               /* field index of DESCRIPTION */

/* The values, one after the other: their offsets. */
#define O_NAME    0
#define O_GENRE   ( O_NAME + L_NAME + 1 )
#define O_YEAR    ( O_GENRE + L_GENRE + 1 )
#define O_PLAYERS ( O_YEAR + L_YEAR + 1 )
#define O_DESC    ( O_PLAYERS + L_PLAYERS + 1 )
#define O_RUN     ( O_DESC + L_DESC + 1 )
#define O_ARGS    ( O_RUN + L_CMD + 1 )
#define O_SETUP   ( O_ARGS + L_CMD + 1 )
#define VALSZ     ( O_SETUP + L_CMD + 1 )

struct Field {
  const char *key, *label;
  int lrow;                     /* the label's row (column 3) */
  int off, max;                 /* the value: offset in Vals, max length, */
  int vrow, vcol, width;        /*   its place and shown width */
  const char *hint;
};

static const Field Fields[NFLD] = {
  { "NAME", "Name", 3, O_NAME, L_NAME, 3, VCOL, L_NAME, "The title in the game list" },
  { "GENRE", "Genre", 4, O_GENRE, L_GENRE, 4, VCOL, L_GENRE,
    "For example: Action, Adventure, Racing, Sports, Strategy" },
  { "YEAR", "Year", 5, O_YEAR, L_YEAR, 5, VCOL, L_YEAR, "The year of the release, for example: 1993" },
  { "PLAYERS", "Players", 6, O_PLAYERS, L_PLAYERS, 6, VCOL, L_PLAYERS,
    "Number of players: a number (2), a range (1-4) or open-ended (2+)" },
  { "DESCRIPTION", "Description", 8, O_DESC, L_DESC, 9, 3, 74,
    "The text of the game page; \\n starts a new line" },
  { "RUN", "Run", 11, O_RUN, L_CMD, 11, VCOL, L_CMD,
    "Ctrl+Enter plays it: a file of the folder, with its arguments" },
  { "ARGS", "Args", 12, O_ARGS, L_CMD, 12, VCOL, L_CMD,
    "Arguments added to RUN, also when it is started from the file list" },
  { "SETUP", "Setup", 13, O_SETUP, L_CMD, 13, VCOL, L_CMD,
    "Ctrl+S runs it: the setup program of the game, with its arguments" }
};

/* code page 437 80h..FFh -> Unicode */
static const unsigned C2u[128] = {
  0xC7, 0xFC, 0xE9, 0xE2, 0xE4, 0xE0, 0xE5, 0xE7, 0xEA, 0xEB, 0xE8, 0xEF, 0xEE, 0xEC, 0xC4, 0xC5,
  0xC9, 0xE6, 0xC6, 0xF4, 0xF6, 0xF2, 0xFB, 0xF9, 0xFF, 0xD6, 0xDC, 0xA2, 0xA3, 0xA5, 0x20A7, 0x192,
  0xE1, 0xED, 0xF3, 0xFA, 0xF1, 0xD1, 0xAA, 0xBA, 0xBF, 0x2310, 0xAC, 0xBD, 0xBC, 0xA1, 0xAB, 0xBB,
  0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
  0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
  0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
  0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
  0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
  0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
  0x3B1, 0xDF, 0x393, 0x3C0, 0x3A3, 0x3C3, 0xB5, 0x3C4, 0x3A6, 0x398, 0x3A9, 0x3B4, 0x221E, 0x3C6, 0x3B5, 0x2229,
  0x2261, 0xB1, 0x2265, 0x2264, 0x2320, 0x2321, 0xF7, 0x2248, 0xB0, 0x2219, 0xB7, 0x221A, 0x207F, 0xB2, 0x25A0, 0xA0
};

static char Vals[VALSZ];        /* the values */
static char Orig[VALSZ];        /*   as they were loaded: only the changed ones are written */
static char IsOld = 0;          /* METADATA.INI exists (RawBuf) */
static char ArgB = 0;           /* /B: started by the menu */
static char GDir[66];           /* the game folder */
static char PathBuf[96];
static char IBuf[IBUFSZ];
static char RawBuf[RAWSZ + 1];
static char OutBuf[2 * RAWSZ + L_DESC * 3 + 1024];   /* kept lines (LF -> CR LF) and new ones */
static char XNames[MAXX][13];   /* executables, sorted */
static int XCount = 0;
static int FCur, ECur, EScr;    /* the active field, the cursor in its value, the first
                                   character shown */
static const char *Msg = NULL;  /* an error shown instead of the hint */

/* scanLine: the line, its field and value */
static const char *LBeg, *LEnd, *VBeg;
static int LFld, VLen;

static char *val( int f ) {
  return Vals + Fields[f].off;
}

static unsigned getKey( void ) {
#ifdef TEST
  unsigned key = testKey( );
  if ( key ) return key;
  testScreen( );
#endif
  return keyRead( );
}

/* An error (exit code 2); started by the menu: after a key. */
static void fail( const char *msg ) {
  say( msg );
  if ( ArgB ) {
    say( "Press a key." );
    getKey( );
  }
  exit( 2 );
}

static void leave( int code ) {
  setTextMode( );
  exit( code );
}

/* /B -> ArgB; the first argument that is not a /switch -> GDir (upper case, without a
   trailing '\'). 0 if there is none. */
static int parseArg( int argc, char *argv[] ) {
  for ( int i = 1; i < argc; i++ ) {
    const char *a = argv[i];
    if ( a[0] == '/' ) {
      if ( upcase( a[1] ) == 'B' ) ArgB = 1;
      continue;
    }
    int n = 0;
    for ( ; *a && n < 64; a++ ) GDir[n++] = upcase( *a );
    while ( n > 1 && GDir[n - 1] == '\\' ) n--;
    GDir[n] = 0;
    return 1;
  }
  return 0;
}

/* name = '\NAME.EXT' -> the game folder + it */
static char *folderFile( char *d, const char *name ) {
  strcpy( d, GDir );
  strcat( d, name );
  return d;
}


/* ---------------------------------------------------------------- METADATA.INI */

static const char *rawStart( void ) {
  return skipBom( RawBuf );
}

static int findField( const char *beg, const char *end ) {
  for ( int f = 0; f < NFLD; f++ ) {
    const char *s = beg, *t = Fields[f].key;
    while ( s != end && upcase( *s ) == *t ) s++, t++;
    if ( s == end && *t == 0 ) return f;
  }
  return -1;
}

/* A line of RawBuf -> LBeg..LEnd (without CR / LF), LFld = the field of a "KEY = value"
   line (-1: another line), VBeg / VLen = its value (trimmed, without surrounding quotes),
   *s = the next line. 0 at the end of the file. */
static int scanLine( const char **ps ) {
  const char *s = *ps, *e;
  LBeg = s;
  if ( *s == 0 || *s == 0x1A ) return 0;
  for ( e = s; !isEol( *e ); e++ );
  LEnd = e;
  if ( *e == 13 ) e++;          /* CR, LF or CR LF */
  if ( *e == 10 ) e++;
  *ps = e;
  LFld = -1;
  s = skipBlank( s );
  if ( *s == ';' || *s == '#' || *s == '[' ) return 1;
  const char *key = s;
  while ( s != LEnd && *s != '=' ) s++;
  if ( s == LEnd ) return 1;
  LFld = findField( key, trimBack( key, s ) );
  s = skipBlank( s + 1 );
  e = trimBack( s, LEnd );
  if ( e - s >= 2 && s[0] == '"' && e[-1] == '"' ) {
    s++;
    e--;
  }
  VBeg = s;
  VLen = e - s;
  return 1;
}

/* VBeg / VLen -> the field (in CP437, clipped); a further DESCRIPTION line is appended
   after \n (as the menu reads it: not after an empty start). */
static void loadValue( int f ) {
  unsigned len = VLen < IBUFSZ - 1 ? VLen : IBUFSZ - 1;
  memcpy( IBuf, VBeg, len );
  IBuf[len] = 0;
  utf8To437( IBuf );
  char *d = val( f );
  char *end = d + Fields[f].max;
  if ( f == F_DESC && *d ) {
    d += strlen( d );
    if ( d + 2 > end ) {
      *d = 0;
      return;
    }
    *d++ = '\\';
    *d++ = 'n';
  }
  for ( const char *s = IBuf; d < end && *s; ) *d++ = *s++;
  *d = 0;
}

/* An existing METADATA.INI -> RawBuf, IsOld = 1, its values -> the fields. */
static void loadIni( void ) {
  int h = open( folderFile( PathBuf, "\\METADATA.INI" ), O_RDONLY | O_BINARY );
  if ( h < 0 ) return;
  int n = read( h, RawBuf, RAWSZ );
  close( h );
  if ( n < 0 ) fail( "GMEDIT: METADATA.INI could not be read.\r\n" );
  if ( n >= RAWSZ ) fail( "GMEDIT: METADATA.INI is too big to edit (max 8 KB).\r\n" );
  RawBuf[n] = 0;
  IsOld = 1;
  const char *s = rawStart( );
  while ( scanLine( &s ) ) {
    if ( LFld >= 0 ) loadValue( LFld );
  }
}


/* ---------------------------------------------------------------- starting values */

/* An executable -> XNames, sorted (skipped when the list is full). */
static void addExe( const char *name ) {
  char x[13];
  memset( x, 0, sizeof( x ) );
  strncpy( x, name, 12 );
  if ( XCount >= MAXX ) return;
  int i;
  for ( i = 0; i < XCount && memcmp( x, XNames[i], 13 ) >= 0; i++ );
  memmove( XNames[i + 1], XNames[i], ( XCount - i ) * 13 );
  memcpy( XNames[i], x, 13 );
  XCount++;
}

static void scanExes( void ) {
  struct find_t f;
  if ( _dos_findfirst( folderFile( PathBuf, "\\*.*" ), _A_NORMAL, &f ) ) return;
  do {
    if ( isExec( f.name ) ) addExe( f.name );
  } while ( _dos_findnext( &f ) == 0 );
}

/* The first line of the text -> NAME (tabs as spaces, without leading and trailing
   spaces, max L_NAME characters). */
static void firstLine( const char *s, int len ) {
  char *d = val( 0 );
  int n = 0;
  while ( len-- > 0 ) {
    char c = *s++;
    if ( isEol( c ) ) break;
    if ( c == 9 ) c = ' ';
    if ( c == ' ' && n == 0 ) continue;
    if ( n >= L_NAME ) break;
    d[n++] = c;
  }
  while ( n && d[n - 1] == ' ' ) n--;
  d[n] = 0;
}

/* SETUP* or INSTALL* */
static int isSetup( const char *name ) {
  return strncmp( name, "SETUP", 5 ) == 0 || strncmp( name, "INSTALL", 7 ) == 0;
}

static void prefill( void ) {
  int n = readFile( folderFile( PathBuf, "\\NAME.TXT" ), IBuf, IBUFSZ - 1 );
  if ( n >= 0 ) {               /* NAME: the first line of NAME.TXT */
    IBuf[n] = 0;
    n = utf8To437( IBuf );
    const char *s = skipBom( IBuf );
    firstLine( s, n - ( s - IBuf ) );
  }
  if ( !*val( 0 ) ) {           /* or the folder name */
    const char *s = GDir;
    for ( const char *p = GDir; *p; p++ ) {
      if ( *p == '\\' || *p == ':' ) s = p + 1;
    }
    strcpy( val( 0 ), s );
  }
  int plays = 0;                /* RUN: the only executable that is not a setup program, */
  const char *run = NULL;       /* SETUP: the first setup program */
  for ( int i = 0; i < XCount; i++ ) {
    if ( isSetup( XNames[i] ) ) {
      if ( !*val( 7 ) ) strcpy( val( 7 ), XNames[i] );
    }
    else {
      plays++;
      run = XNames[i];
    }
  }
  if ( plays == 1 ) strcpy( val( 5 ), run );
}


/* ---------------------------------------------------------------- saving */

static int isBlank( const char *s ) {
  while ( *s == ' ' ) s++;
  return *s == 0;
}

/* Text in code page 437 -> UTF-8 (without the terminating 0). */
static char *putUtf8( char *d, const char *s ) {
  for ( ; *s; s++ ) {
    unsigned char c = *s;
    if ( c < 0x80 ) {
      *d++ = c;
      continue;
    }
    unsigned u = C2u[c - 0x80];
    if ( u < 0x800 ) *d++ = (char)( 0xC0 | ( u >> 6 ) );
    else {
      *d++ = (char)( 0xE0 | ( u >> 12 ) );
      *d++ = (char)( 0x80 | ( ( u >> 6 ) & 0x3F ) );
    }
    *d++ = (char)( 0x80 | ( u & 0x3F ) );
  }
  return d;
}

/* "KEY=value" line (none if the value is blank) */
static char *putField( char *d, int f ) {
  const char *v = val( f );
  if ( isBlank( v ) ) return d;
  strcpy( d, Fields[f].key );
  d += strlen( d );
  *d++ = '=';
  d = putUtf8( d, v );
  *d++ = 13;
  *d++ = 10;
  return d;
}

/* 0 when METADATA.INI was written; otherwise the message is set. */
static int save( void ) {
  char state[NFLD];             /* 0 = unchanged, 1 = changed, 2 = written */
  char *d = OutBuf;
  int f, changed = 0;
  for ( f = 0; f < NFLD; f++ ) {
    state[f] = strcmp( val( f ), Orig + Fields[f].off ) != 0;
    changed |= state[f];
  }
  if ( !IsOld ) {
    strcpy( d, "[GAME]\r\n" );
    d += strlen( d );
  }
  else {
    if ( !changed ) leave( 1 );                /* nothing changed: nothing to write */
    const char *s = rawStart( );
    memcpy( d, RawBuf, s - RawBuf );           /* the BOM is kept */
    d += s - RawBuf;
    while ( scanLine( &s ) ) {
      if ( LFld >= 0 && state[LFld] ) {
        if ( state[LFld] == 1 ) {              /* written already: a further line of the */
          state[LFld] = 2;                     /*   key is dropped */
          d = putField( d, LFld );
        }
        continue;
      }
      memcpy( d, LBeg, LEnd - LBeg );
      d += LEnd - LBeg;
      *d++ = 13;
      *d++ = 10;
    }
  }
  for ( f = 0; f < NFLD; f++ ) {               /* the changed values the file had no line for */
    if ( state[f] == 1 ) d = putField( d, f );
  }
  char tmp[96], ini[96];
  folderFile( tmp, "\\METADATA.$$$" );
  folderFile( ini, "\\METADATA.INI" );
  if ( writeFile( tmp, OutBuf, d - OutBuf ) ) {
    remove( tmp );                             /* a partly written file is deleted */
  }
  else {
    remove( ini );
    if ( rename( tmp, ini ) == 0 ) return 0;
  }
  Msg = "METADATA.INI could not be written (the disk is full or write-protected)";
  beep( );
  return -1;
}


/* ---------------------------------------------------------------- editing */

/* The cursor goes to the end of the field's value. */
static void setField( int f ) {
  FCur = f;
  ECur = strlen( val( f ) );
  EScr = 0;
}

/* The character is inserted at the cursor (a beep if the value is full). */
static void insChar( char c ) {
  char *v = val( FCur );
  int len = strlen( v );
  if ( len >= Fields[FCur].max ) {
    beep( );
    return;
  }
  memmove( v + ECur + 1, v + ECur, len - ECur + 1 );
  v[ECur++] = c;
}

/* The label and the value (the active one scrolled to the cursor). */
static void drawField( int f ) {
  const Field &F = Fields[f];
  putAt( F.lrow, 3, F.label, VCOL - 4, 0x1B );
  const char *v = val( f );
  unsigned char attr = 0x07;
  if ( f == FCur ) {            /* EScr: the cursor must be within the width */
    if ( ECur < EScr ) EScr = ECur;
    if ( ECur - EScr >= F.width ) EScr = ECur - F.width + 1;
    v += EScr;
    attr = 0x70;
  }
  int pos = scrAt( F.vrow, F.vcol );
  int n = 0;
  for ( ; n < F.width && v[n]; n++ ) pos = putCh( pos, v[n], attr );
  for ( ; n < F.width; n++ ) pos = putCh( pos, ' ', attr );
}

static void draw( void ) {
  union REGS r;
  int pos = putStr( 0, IsOld ? " Edit METADATA.INI: " : " New METADATA.INI: ", 20, 0x70 );
  pos = putStr( pos, GDir, 60, 0x70 );
  fillTo( pos, COLS, 0x70 );
  drawBox( 1, 0, 23, 79, 0x13 );
  boxTitle( 1, 2, "Game data", 30, 0x1E );
  for ( int f = 0; f < NFLD; f++ ) drawField( f );
  hsep( 15, 0, 79, 0x13 );      /* the executables of the folder */
  boxTitle( 15, 2, "Executables", 20, 0x1E );
  if ( XCount == 0 ) putAt( 16, 3, "(no executables)", 20, 0x17 );
  for ( int i = 0; i < XCount && i < XCOLS * XROWS; i++ ) {
    putAt( 16 + i / XCOLS, 3 + i % XCOLS * XW, XNames[i], 12, 0x1F );
  }
  hsep( 21, 0, 79, 0x13 );      /* the hint of the field, or the error */
  if ( Msg ) putAt( 22, 3, Msg, 74, 0x1E );
  else putAt( 22, 3, Fields[FCur].hint, 74, 0x17 );
  pos = putCh( 24 * COLS, ' ', 0x70 );
  pos = putFooter( pos, "~Tab~/~\x18\x19~ Field  ~F2~ Save  ~Esc~ Cancel  ~Ctrl+Y~ Clear" );
  fillTo( pos, COLS * ROWS, 0x70 );
  r.h.ah = 2;                   /* the cursor */
  r.h.bh = 0;
  r.h.dh = (unsigned char)Fields[FCur].vrow;
  r.h.dl = (unsigned char)( Fields[FCur].vcol + ECur - EScr );
  int86( 0x10, &r, &r );
}

static void edit( void ) {
  for ( ;; ) {
    draw( );
    unsigned k = getKey( );
    unsigned char al = (unsigned char)k, ah = (unsigned char)( k >> 8 );
    char *v = val( FCur );
    Msg = NULL;
    if ( al == 27 ) leave( 1 );
    else if ( al == 9 || al == 13 ) setField( ( FCur + 1 ) % NFLD );
    else if ( al == 8 ) {
      if ( ECur ) {
        ECur--;
        memmove( v + ECur, v + ECur + 1, strlen( v + ECur ) );
      }
    }
    else if ( al == 0x19 ) {    /* Ctrl+Y: clear the value */
      v[0] = 0;
      ECur = 0;
    }
    else if ( al == 0 || ( al == 0xE0 && ah ) ) {   /* E0h with a scan code: a grey key */
      switch ( ah ) {
        case 0x0F:              /* Shift+Tab */
        case 0x48:
          setField( ( FCur + NFLD - 1 ) % NFLD );
          break;
        case 0x50:
          setField( ( FCur + 1 ) % NFLD );
          break;
        case 0x4B:
          if ( ECur ) ECur--;
          break;
        case 0x4D:
          if ( v[ECur] ) ECur++;
          break;
        case 0x47:
          ECur = 0;
          break;
        case 0x4F:
          setField( FCur );
          break;
        case 0x53:
          if ( v[ECur] ) memmove( v + ECur, v + ECur + 1, strlen( v + ECur ) );
          break;
        case 0x3C:              /* F2: save */
          if ( save( ) == 0 ) leave( 0 );
          break;
      }
    }
    else if ( al >= ' ' && al != 0x7F ) insChar( al );
  }
}

int main( int argc, char *argv[] ) {
  struct find_t f;
  getProgDir( argv[0] );
  if ( !parseArg( argc, argv ) ) fail( "Usage: GMEDIT <game folder>   (creates or edits its METADATA.INI)\r\n" );
  if ( _dos_findfirst( GDir, _A_SUBDIR, &f ) || !( f.attrib & _A_SUBDIR ) ) {
    fail( "GMEDIT: the game folder was not found.\r\n" );
  }
  loadIni( );
  memcpy( Orig, Vals, VALSZ );
  scanExes( );
  if ( !IsOld ) prefill( );
  setTextMode( );
  setField( 0 );
  edit( );
  return 0;
}
