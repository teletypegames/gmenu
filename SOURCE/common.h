/*
   common.h - shared by GMENUPRG.EXE and GMEDIT.EXE: the METADATA.INI limits, strings,
   the program's folder, files, the text screen, UTF-8, the keyboard and the test hooks.
*/

#ifndef COMMON_H
#define COMMON_H

/* Max lengths of the METADATA.INI values. */
#define L_NAME    50
#define L_GENRE   24
#define L_YEAR    10
#define L_PLAYERS 10
#define L_DESC    2000
#define L_CMD     60            /* RUN, SETUP, ARGS */

#define COLS 80
#define ROWS 25

/* The folder of the program, with a trailing '\', and the current folder at the start
   (without the drive and the leading '\'). */
extern char ProgDir[80];
extern char StartDir[66];

char upcase( char c );
/* Like stricmp, but only a-z is upper-cased, and the bytes compare unsigned. */
int  cmpi( const char *a, const char *b );
int  isEol( char c );           /* 0, CR, LF, EOF (1Ah) */
const char *skipBlank( const char *s );
const char *trimBack( const char *beg, const char *end );
/* .COM / .EXE / .BAT */
int  isExec( const char *name );

/* ProgDir and StartDir; argv0 is the path of the program. */
void getProgDir( const char *argv0 );
/* ProgDir + name, in a buffer that the next call overwrites. */
const char *progFile( const char *name );

/* The bytes read (at most size), or -1 if the file cannot be opened or read. */
int  readFile( const char *path, void *buf, unsigned size );
/* A new file with len bytes: 0, or -1 on an error. */
int  writeFile( const char *path, const void *buf, unsigned len );

/* Text on the standard output; a beep. */
void say( const char *s );
void beep( void );

/* The 80x25 text screen. A position is the cell index (row * 80 + column); the drawing
   routines return the position after what they wrote. */
void setTextMode( void );
int  scrAt( int row, int col );
int  putCh( int pos, char c, unsigned char attr );
int  putStr( int pos, const char *s, int max, unsigned char attr );
int  fillTo( int pos, int end, unsigned char attr );
int  putAt( int row, int col, const char *s, int max, unsigned char attr );
int  putNum( int pos, unsigned n, unsigned char attr );
/* " text " on a frame. */
int  boxTitle( int row, int col, const char *s, int max, unsigned char attr );
/* A double-line frame, and a separator line inside one. */
void drawBox( int top, int left, int bottom, int right, unsigned char attr );
void hsep( int row, int left, int right, unsigned char attr );
/* The footer text: '~' toggles the highlight color. */
int  putFooter( int pos, const char *s );

/* UTF-8 -> code page 437 in place (s is null-terminated): the new length. Bytes that do
   not form a valid UTF-8 sequence are kept (text already in CP437); characters missing
   from CP437 become their base letter or '?', the BOM is dropped. */
unsigned utf8To437( char *s );
/* After a UTF-8 BOM. */
const char *skipBom( const char *s );

/* Decimal digits of n at d, null-terminated: the position of the 0. */
char *numStr( char *d, unsigned n );

/* BIOS key code: scan code << 8 | ASCII. */
unsigned keyRead( void );
int  keyReady( void );
int  shiftDown( void );

#ifdef TEST
/* Test builds: the keys come from TESTKEYS.TXT in the program's folder (hex BIOS codes,
   e.g. "5000 1C0D"); 0 when they ran out. testScreen saves the text screen to SCREEN.BIN
   in the program's folder and exits with code 3. */
unsigned testKey( void );
void testScreen( void );
#endif

#endif
