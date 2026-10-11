/*
   gmenu.h - the parts of GMENUPRG.EXE that its modules share.

   A game is an index into Games, in the order the folder scan found them; GPtrs holds
   them in the sort order, VPtrs the ones the search and the filters let through.
*/

#ifndef GMENU_H
#define GMENU_H

#include "common.h"

#define MAXG    600             /* max games */
#define MAXF    600             /* max files per folder */
#define FSZ     13              /* filename size (8.3 + 0) */
#define GROWS   22              /* game list rows (2..23) */
#define FROWS   21              /* file list rows in the detail view (2..22) */
#define PROWS   21              /* picker list rows (2..22) */
#define DROWS   14              /* description rows in the detail view (9..22) */
#define HROWS   21              /* help rows (2..22) */
#define HKW     16              /* help: key cap width */
#define MAXP    250             /* max values in the picker */
#define NREC    10              /* recently played games */
#define MAXPL   16              /* players filter: highest player count listed */
#define NSORT   4               /* sort orders (F9): title, year, genre, most played */
#define WSTEP   3               /* mouse wheel: items per notch */
#define POOLSZ  6000            /* picker values */
#define IBUFSZ  4096            /* METADATA.INI / NAME.TXT read buffer */
#define DBLT    9               /* double click: max ticks (1/18.2 s) between the clicks */

/* The fields of a game's record, after NAME. RUN / SETUP: command line (file
   [arguments]), ARGS: arguments for RUN. */
enum { F_GENRE, F_YEAR, F_PLAYERS, F_DESC, F_RUN, F_SETUP, F_ARGS };
#define NKEYS   8               /* METADATA.INI keys: NAME + the record fields */

#define C_TITLE  1              /* game list columns */
#define W_TITLE  44
#define C_GENRE  46
#define W_GENRE  15
#define C_YEAR   62
#define W_YEAR   4
#define C_FOLDER 67
#define W_FOLDER 12

#define LBOXR   55              /* detail view: right edge of the left box */
#define FCOL    57              /* file list first column (inside the right box) */

#define G_FAV     1             /* Game.flags: favorite, */
#define G_NAMETXT 2             /*   title not from METADATA.INI, */
#define G_NOINI   4             /*   no METADATA.INI */

/* Input besides the BIOS key codes; a right click is Esc. */
#define K_CLICK    0xFF01
#define K_WHEELUP  0xFF02
#define K_WHEELDN  0xFF03
#define K_DBLCLICK 0xFF04       /* runList: double click in the game list */

struct Game {
  char folder[FSZ];
  char title[L_NAME + 1];
  char *rec;                    /* genre,0,year,0,players,0,desc,0,run,0,setup,0,args,0 or NULL */
  unsigned char stamp[16];      /* METADATA.INI, NAME.TXT: time, date, size (cache.cpp) */
  unsigned char flags;
  unsigned plays;               /* starts, date of the last start (DOS format) */
  unsigned last;
};

/* The list widget (ui.cpp): ListMode 0 = game list, 1 = detail view, 2 = picker. */
struct List {
  int count, sel, top, row0, rows;
  const char *header;
};
enum { L_SELECTED, L_ESC, L_KEY };

/* GMENU.CPP */
extern char ColDir[66];
extern const char *CdPath;
extern char GDrive, HDrive;
extern char HdrGames[96];
extern char HdrBuf[200];
extern char DirBuf[80];
void quit( void );

/* catalog.cpp */
extern Game *Games;
extern int GCount;
extern int GPtrs[MAXG];
extern char PathBuf[128];
extern unsigned char Stamp[16];
extern int GSrc;
void scanGames( void );
const char *getField( int g, int field );
const char *gamePath( int g, const char *name );
char *putColDir( char *d );

/* cache.cpp */
void idxOpen( void );
int  idxLookup( int g );
void keepStamp( int g );
void idxFinish( void );
void fileStamp( int g, const char *name, unsigned char *out );

/* filter.cpp */
extern int VPtrs[MAXG];
extern int VCount, GSel, GTop;
extern char FBuf[32];
extern unsigned char FLen;
extern char FGenre[L_GENRE + 1], FYear[L_YEAR + 1], FPlayers[L_PLAYERS + 1];
extern char FavOnly, NoIni;
extern int Keep;
extern const char *PPtrs[MAXP];
extern int PCount;
extern int PLeft, PRight;
extern const char *PTitle;
void filterAdd( char c );
void filterBack( void );
int  filtersActive( void );
void clearFilters( void );
void filterClear( void );
void refilter( void );
void findKeep( void );
void applyFilter( void );
void pickFilter( int field, char *dest, const char *title );
int  parseNum( const char **s, unsigned *n );
int  parseRange( const char **s, unsigned *a, unsigned *b );

/* favorites.cpp */
extern char Recent[NREC][FSZ];
int  isFav( int g );
void toggleFav( int g );
void loadFavs( void );
int  findEntry( const char *folder );
void addRecent( int g );
int  pickRecent( int *g );

/* stats.cpp */
void loadStats( void );
void addPlay( int g );
const char *playedStr( int g );
int  cmpPlays( int a, int b );

/* session.cpp */
void loadSel( void );
void saveSel( void );

/* sort.cpp */
extern char SortMode;
extern const char *const SortNames[NSORT];
void sortGames( void );
int  cmpTitle( int a, int b );
template <class T> void shellSort( T *a, int n, int ( *cmp )( T, T ) ) {
  static const int gaps[] = { 701, 301, 132, 57, 23, 10, 4, 1, 0 };
  for ( const int *gap = gaps; *gap; gap++ ) {
    if ( *gap >= n ) continue;
    for ( int i = *gap; i < n; i++ ) {
      T key = a[i];
      int j = i;
      for ( ; j >= *gap && cmp( a[j - *gap], key ) > 0; j -= *gap ) a[j] = a[j - *gap];
      a[j] = key;
    }
  }
}

/* launcher.cpp */
extern int CurGame;
void launch( void );
void editIni( int g );
void play( int g );
void setup( int g );
void fileCmd( const char *file );
int  findRunFile( void );
int  tokEq( const char *cmd, const char *name );

/* ui.cpp */
extern List L;
extern char ListMode;
extern char HasMouse, HasWheel;
extern char ShowAll;
extern int DScroll, DLines;
void restoreScreen( void );
void mouseShow( void );
int  runList( unsigned *key );
void showHelp( void );
unsigned ticks( void );

/* browser.cpp */
extern char FNames[MAXF][FSZ];
extern const char *FPtrs[MAXF];
extern int FCount;
void gameLoop( void );
void detail( int g );
void scanFiles( void );

#endif
