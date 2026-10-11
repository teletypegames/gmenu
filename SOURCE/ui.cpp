/*
   ui.cpp - the screen, the mouse, the list widget, the input, the drawing.
*/

#include <i86.h>
#include <string.h>

#include "gmenu.h"

List L;
char ListMode = 0;
char HasMouse = 0, HasWheel = 0;
char ShowAll = 0;               /* all files of a game, not just the executables */
int DScroll = 0;                /* detail view: description lines scrolled off the top */
int DLines = 0;                 /*   all wrapped description lines */

static int MRow = 0, MCol = 0;  /* the place of the last click */
static unsigned char PrevBtn = 0;
static char MDbl = 0;           /* the last click was a double click */
static unsigned LClickT = 0;    /* time and row of the previous click */
static int LClickR = -1;
static unsigned char RAttr;     /* color of the row being drawn */

static const char StrNoMatch[] = "No games match the search / filters.  (Esc: clear, F5: clear filters)";
static const char FooterG[] = "~Type~ Search ~Ctrl+Enter~ Play ~F1~ Help ~F2-4~ Filter ~F6/7~ Fav ~F9~ Sort ~F10~ Random";
static const char FooterF[] = "~\x18\x19~ Move ~Enter~ Run ~Tab~ All files ~F1~ Help ~F6~ Fav ~Ctrl+PgUp/Dn~ Text ~Esc~ Back";
static const char FooterFA[] = "~\x18\x19~ Move ~Enter~ Run ~Tab~ Executables ~F1~ Help ~F6~ Fav ~Ctrl+PgUp/Dn~ Text ~Esc~ Back";
static const char FooterP[] = "~\x18\x19~ Move  ~Enter~ Select  ~A-Z~ Jump  ~Esc~ Cancel";
static const char FooterH[] = "~\x18\x19 PgUp PgDn~ Scroll  ~Esc~ Close";

static void mouse( union REGS *r ) {
  int86( 0x33, r, r );
}

void restoreScreen( void ) {
  union REGS r;
  if ( HasMouse ) {
    r.w.ax = 0;
    mouse( &r );
  }
  setTextMode( );
}

void mouseShow( void ) {
  union REGS r;
  if ( !HasMouse ) return;
  r.w.ax = 1;
  mouse( &r );
}

static void mouseHide( void ) {
  union REGS r;
  if ( !HasMouse ) return;
  r.w.ax = 2;
  mouse( &r );
}

unsigned ticks( void ) {
  return *(unsigned far *)MK_FP( 0x40, 0x6C );
}


/* ---------------------------------------------------------------- input */

/* A key: its BIOS code; a left click: K_CLICK, MRow / MCol, MDbl; a right click: Esc;
   the wheel: K_WHEELUP / K_WHEELDN. */
static unsigned getInput( void ) {
  union REGS r;
#ifdef TEST
  unsigned key = testKey( );
  if ( key ) return key;
  mouseHide( );                 /* the mouse cursor is not part of the screen */
  testScreen( );
#endif
  for ( ;; ) {
    r.w.ax = 0x1680;            /* nothing to do: the time slice to a multitasker */
    int86( 0x2F, &r, &r );
    if ( keyReady( ) ) return keyRead( );
    if ( !HasMouse ) continue;
    r.w.ax = 3;
    mouse( &r );
    signed char wheel = (signed char)r.h.bh;
    MRow = r.w.dx >> 3;
    MCol = r.w.cx >> 3;
    unsigned char pressed = r.h.bl & ~PrevBtn;
    PrevBtn = r.h.bl;
    if ( pressed & 1 ) {        /* double click: the same row, within DBLT ticks */
      unsigned t = ticks( );
      MDbl = 0;
      if ( t - LClickT < DBLT && MRow == LClickR ) {
        MDbl = 1;
        t -= DBLT;              /* a third click starts a new pair */
      }
      LClickT = t;
      LClickR = MRow;
      return K_CLICK;
    }
    if ( pressed & 2 ) return 0x011B;
    if ( HasWheel && wheel ) return wheel < 0 ? K_WHEELUP : K_WHEELDN;
  }
}

/* After a click on the selected game: 1 if a second click follows within DBLT ticks. */
static int waitDbl( void ) {
  union REGS r;
  r.w.ax = 5;                   /* clears the press counter */
  r.w.bx = 0;
  mouse( &r );
  unsigned t = ticks( );
  for ( ;; ) {
    r.w.ax = 5;
    r.w.bx = 0;
    mouse( &r );
    if ( r.w.bx ) break;
    if ( ticks( ) - t >= DBLT ) return 0;
  }
  LClickR = -1;
  r.w.ax = 3;
  mouse( &r );
  PrevBtn = r.h.bl;
  return 1;
}


/* ---------------------------------------------------------------- list */

static const char *getItem( int i ) {
  if ( ListMode == 1 ) return FPtrs[i];
  if ( ListMode == 2 ) return PPtrs[i];
  return Games[VPtrs[i]].folder;
}

static void fixTop( void ) {
  if ( L.sel < L.top ) L.top = L.sel;
  if ( L.sel >= L.top + L.rows ) L.top = L.sel - L.rows + 1;
}

static void clampSel( void ) {
  if ( L.count == 0 ) L.sel = 0;
  else if ( L.sel >= L.count ) L.sel = L.count - 1;
  fixTop( );
}

/* The next item that starts with the character. */
static void jumpLetter( char c ) {
  c = upcase( c );
  int i = L.sel;
  for ( int n = L.count; n; n-- ) {
    if ( ++i >= L.count ) i = 0;
    if ( upcase( getItem( i )[0] ) == c ) {
      L.sel = i;
      return;
    }
  }
}

/* The wheel turned over the left box of the detail view. */
static int wheelLeft( void ) {
  return ListMode == 1 && MCol < FCOL - 1;
}

static void drawList( void );

/* The list in L and ListMode: L_SELECTED (L.sel), L_ESC, or L_KEY with the key, or
   K_DBLCLICK in the game list. */
int runList( unsigned *key ) {
  unsigned k;
  unsigned char al, ah;
  int b, r;
  clampSel( );
redraw:
  drawList( );
wait:
  k = getInput( );
  b = L.sel;
  if ( k == K_CLICK ) goto click;
  if ( k == K_WHEELUP || k == K_WHEELDN ) {     /* the wheel moves without wrapping */
    if ( wheelLeft( ) ) goto other;
    if ( k == K_WHEELUP ) {
      b -= WSTEP;
      if ( b < 0 ) b = 0;
    }
    else {
      b += WSTEP;
      if ( b >= L.count ) b = L.count - 1;
    }
    goto setsel;
  }
  al = (unsigned char)k;
  ah = (unsigned char)( k >> 8 );
  if ( al == 27 ) return L_ESC;
  if ( al == 13 ) goto enter;
  if ( al == 0 || al == 0xE0 ) {
    /* detail view: Shift+PgUp/PgDn scrolls the description */
    if ( ListMode == 1 && ( ah == 0x49 || ah == 0x51 ) && shiftDown( ) ) goto other;
    switch ( ah ) {
      case 0x48:                /* the top wraps to the last item */
        b = b ? b - 1 : L.count - 1;
        goto setsel;
      case 0x50:                /* the bottom wraps to the first item */
        if ( ++b >= L.count ) b = 0;
        goto setsel;
      case 0x49:
        b -= L.rows;
        if ( b < 0 ) b = 0;
        goto setsel;
      case 0x51:
        b += L.rows;
        if ( b >= L.count ) b = L.count - 1;
        goto setsel;
      case 0x47:
        b = 0;
        goto setsel;
      case 0x4F:
        b = L.count - 1;
        goto setsel;
      case 0x4B:
        return L_ESC;
      case 0x4D:
        goto enter;
    }
    goto other;
  }
  if ( al <= ' ' || ListMode == 0 ) goto other;   /* game list: the search (caller) */
  jumpLetter( al );
  goto moved;

setsel:
  if ( L.count == 0 ) goto wait;
  L.sel = b;
moved:
  fixTop( );
  goto redraw;

click:
  if ( MRow == 0 ) {            /* the header / footer: page up / down */
    b = L.sel - L.rows;
    if ( b < 0 ) b = 0;
    goto setsel;
  }
  if ( MRow >= 24 ) {
    b = L.sel + L.rows;
    if ( b >= L.count ) b = L.count - 1;
    goto setsel;
  }
  r = MRow - L.row0;
  if ( r < 0 || r >= L.rows ) goto wait;
  if ( ListMode == 1 && MCol < FCOL - 1 ) goto wait;   /* detail view: only the file list */
  r += L.top;
  if ( r >= L.count ) goto wait;
  if ( ListMode == 0 ) {        /* a click selects, a click on the selected game opens */
    if ( MDbl ) {               /*   it, a double click plays */
      L.sel = r;
      k = K_DBLCLICK;
      goto other;
    }
    if ( r == L.sel ) {
      if ( !waitDbl( ) ) goto enter;
      k = K_DBLCLICK;
      goto other;
    }
    L.sel = r;
    goto moved;
  }
  L.sel = r;
  drawList( );
enter:
  if ( L.count == 0 ) goto wait;
  return L_SELECTED;
other:
  *key = k;
  return L_KEY;
}


/* ---------------------------------------------------------------- drawing */

static int colStr( int pos, int row0, int col, const char *s, int width, unsigned char attr ) {
  pos = fillTo( pos, row0 + col, attr );
  return putStr( pos, s, width, attr );
}

/* The column color, or the selection color in the selected row. */
static unsigned char rowAttr( unsigned char attr ) {
  return RAttr == 0x30 ? 0x30 : attr;
}

/* The game list table: Title / Genre / Year / Folder */
static void drawGames( void ) {
  int pos = COLS, row0 = COLS;
  pos = colStr( pos, row0, C_TITLE, "Title", W_TITLE, 0x1E );
  pos = colStr( pos, row0, C_GENRE, "Genre", W_GENRE, 0x1E );
  pos = colStr( pos, row0, C_YEAR, "Year", W_YEAR, 0x1E );
  pos = colStr( pos, row0, C_FOLDER, "Folder", W_FOLDER, 0x1E );
  fillTo( pos, row0 + COLS, 0x1E );
  for ( int d = 0; d < L.rows; d++ ) {
    int i = L.top + d;
    unsigned char a = 0x1F;
    pos = row0 = scrAt( L.row0 + d, 0 );
    if ( i < L.count ) {
      int g = VPtrs[i];
      if ( i == L.sel ) a = 0x30;
      RAttr = a;
      pos = colStr( pos, row0, C_TITLE, Games[g].title, W_TITLE, a );
      pos = colStr( pos, row0, C_GENRE, getField( g, F_GENRE ), W_GENRE, rowAttr( 0x1B ) );
      pos = colStr( pos, row0, C_YEAR, getField( g, F_YEAR ), W_YEAR, rowAttr( 0x1E ) );
      pos = colStr( pos, row0, C_FOLDER, Games[g].folder, W_FOLDER, rowAttr( 0x17 ) );
      if ( isFav( g ) ) {       /* a favorite: yellow '*' in the last column */
        pos = fillTo( pos, row0 + 79, a );
        pos = putCh( pos, '*', ( a & 0xF0 ) | 0x0E );
      }
    }
    fillTo( pos, row0 + COLS, a );
  }
  if ( L.count == 0 ) putAt( 3, 3, StrNoMatch, 70, 0x17 );
}

/* Text with word wrap; 0Ah is a line break. The first skip lines are not shown
   (scrolling): the number of all lines. */
static int wrapText( const char *s, int row, int col, int rows, int width, unsigned char attr, int skip ) {
  int total = 0;
  for ( ;; ) {
    while ( *s == ' ' ) s++;
    if ( *s == 0 ) return total;
    int n = 0, space = 0;       /* the position in the line, the last space (0 = none) */
    for ( ;; ) {
      char c = s[n];
      if ( c == 0 || c == 10 ) break;
      if ( n >= width ) {
        if ( c != ' ' && space ) n = space;   /* no space: a hard break */
        break;
      }
      if ( c == ' ' ) space = n;
      n++;
    }
    if ( total >= skip && total - skip < rows ) {
      int pos = scrAt( row + total - skip, col );
      for ( int i = 0; i < n; i++ ) pos = putCh( pos, s[i], attr );
    }
    s += n;
    total++;
    if ( *s == 10 ) s++;
  }
}

static void infoLine( int row, int field, const char *label ) {
  putAt( row, 3, label, 10, 0x1B );
  const char *v = getField( CurGame, field );
  if ( *v ) putAt( row, 13, v, LBOXR - 14, 0x1F );
  else putAt( row, 13, "-", LBOXR - 14, 0x17 );
}

/* The detail view: the data on the left, the file list on the right. */
static void drawDetail( void ) {
  drawBox( 1, 0, 23, LBOXR, 0x13 );
  int pos = boxTitle( 1, 2, Games[CurGame].title, LBOXR - 7, 0x1E );
  if ( isFav( CurGame ) ) {     /* a favorite: " Title * " */
    pos = putCh( pos, '*', 0x1E );
    putCh( pos, ' ', 0x1E );
  }
  hsep( 8, 0, LBOXR, 0x13 );
  infoLine( 3, F_GENRE, "Genre" );
  infoLine( 4, F_YEAR, "Year" );
  infoLine( 5, F_PLAYERS, "Players" );
  putAt( 6, 3, "Folder", 10, 0x1B );
  putAt( 6, 13, DirBuf, LBOXR - 14, 0x1F );
  putAt( 7, 3, "Played", 10, 0x1B );
  putAt( 7, 13, playedStr( CurGame ), LBOXR - 14, 0x1F );
  const char *desc = getField( CurGame, F_DESC );
  if ( *desc ) DLines = wrapText( desc, 9, 3, DROWS, LBOXR - 4, 0x17, DScroll );
  else DLines = wrapText( "No description.", 9, 3, DROWS, LBOXR - 4, 0x18, DScroll );
  if ( DScroll ) putCh( scrAt( 8, LBOXR - 2 ), 0x18, 0x13 );   /* more above / below */
  if ( DLines - DScroll > DROWS ) putCh( scrAt( 23, LBOXR - 2 ), 0x19, 0x13 );

  drawBox( 1, LBOXR + 1, 23, 79, 0x13 );
  boxTitle( 1, LBOXR + 3, ShowAll ? "All files" : "Executables", 18, 0x1E );
  if ( L.count == 0 ) {
    putAt( 2, FCOL + 1, ShowAll ? "(no files)" : "(no executables)", 20, 0x17 );
    if ( !ShowAll ) putAt( 4, FCOL + 1, "Tab: all files", 20, 0x17 );
    return;
  }
  const char *run = getField( CurGame, F_RUN );
  for ( int d = 0; d < FROWS && L.top + d < L.count; d++ ) {
    int i = L.top + d;
    const char *name = getItem( i );
    int exec = !ShowAll || isExec( name );
    unsigned char a = ( i == L.sel ) ? 0x30 : 0x1F;
    unsigned char ma = 0x1A;
    char mark = ' ';
    RAttr = a;
    if ( ShowAll && exec ) mark = 0x10;   /* all files: a marker before the executables */
    if ( tokEq( run, name ) ) {           /* the RUN file: a yellow marker */
      mark = 0x10;
      ma = 0x1E;
    }
    pos = putCh( scrAt( d + 2, FCOL ), mark, rowAttr( ma ) );
    pos = putStr( pos, name, 12, exec ? a : rowAttr( 0x17 ) );
    fillTo( pos, scrAt( d + 2, 79 ), exec ? a : rowAttr( 0x17 ) );
  }
}

/* The picker: a box with the values. */
static void drawPicker( void ) {
  fillTo( COLS, COLS * 24, 0x17 );
  drawBox( 1, PLeft, 23, PRight, 0x13 );
  boxTitle( 1, PLeft + 2, PTitle, PRight - PLeft - 5, 0x1E );
  for ( int d = 0; d < PROWS && L.top + d < L.count; d++ ) {
    int i = L.top + d;
    unsigned char a = ( i == L.sel ) ? 0x30 : 0x1F;
    int pos = putCh( scrAt( d + 2, PLeft + 1 ), ' ', a );
    pos = putStr( pos, getItem( i ), PRight - PLeft - 3, a );
    fillTo( pos, scrAt( d + 2, PRight ), a );
  }
}

static void drawFooter( void ) {
  int pos = putCh( 24 * COLS, ' ', 0x70 );
  if ( ListMode == 1 ) pos = putFooter( pos, ShowAll ? FooterFA : FooterF );
  else if ( ListMode == 2 ) pos = putFooter( pos, FooterP );
  else if ( FLen == 0 ) pos = putFooter( pos, FooterG );
  else {                        /* the search line */
    pos = putStr( pos, "Search: ", 20, 0x1E );
    pos = putStr( pos, FBuf, 30, 0x1F );
    pos = putCh( pos, '_', 0x1F );
    pos = putStr( pos, "   (Backspace: delete, Esc: clear search)", 79, 0x1E );
    fillTo( pos, COLS * ROWS, 0x1E );
    return;
  }
  fillTo( pos, COLS * ROWS, 0x70 );
}

static void drawList( void ) {
  mouseHide( );
  int pos = putCh( 0, ' ', 0x70 );
  pos = putStr( pos, L.header, 64, 0x70 );
  pos = fillTo( pos, 68, 0x70 );
  pos = putNum( pos, L.count ? L.sel + 1 : L.sel, 0x70 );
  pos = putCh( pos, '/', 0x70 );
  pos = putNum( pos, L.count, 0x70 );
  fillTo( pos, COLS, 0x70 );
  if ( ListMode == 1 ) drawDetail( );
  else if ( ListMode == 2 ) drawPicker( );
  else drawGames( );
  drawFooter( );
  mouseShow( );
}


/* ---------------------------------------------------------------- help */

/* A section title (key NULL), an empty row (both NULL), or a key and its description. */
static const struct { const char *key, *desc; } HelpTxt[] = {
  { NULL, "Game list" },
  { "Enter, click", "open the game page" },
  { "Ctrl+Enter", "play (or double click)" },
  { "Ctrl+S", "run the setup" },
  { "Ctrl+E", "edit METADATA.INI" },
  { "Typing", "search" },
  { "F2  F3  F4", "filter: genre, year, players" },
  { "F5", "clear the filters" },
  { "F6  F7", "favorite, favorites only" },
  { "Shift+F7", "no METADATA.INI only" },
  { "F8", "recently played" },
  { "F9  F10", "sort order, random game" },
  { "Esc", "clear search, filters, quit" },
  { NULL, NULL },
  { NULL, "Game page" },
  { "Enter, click", "run the file" },
  { "Tab", "all files / executables" },
  { "A-Z  0-9", "jump to a file" },
  { "Ctrl+PgUp/Dn", "scroll the description" },
  { "F6", "favorite" },
  { "Ctrl+E", "edit METADATA.INI" },
  { "Esc  \x1B", "back to the list" }
};
#define NHELP ( sizeof( HelpTxt ) / sizeof( HelpTxt[0] ) )

/* The rows from top: the section titles with a line, the keys as caps, the
   descriptions. */
static void drawHelp( int top ) {
  mouseHide( );
  drawBox( 1, 4, 23, 75, 0x13 );
  boxTitle( 1, 6, "Keys", 20, 0x1E );
  for ( int i = top; i < NHELP && i - top < HROWS; i++ ) {
    int row = 2 + i - top;
    int pos;
    if ( HelpTxt[i].key ) {
      pos = putCh( scrAt( row, 7 ), ' ', 0x30 );
      pos = putStr( pos, HelpTxt[i].key, HKW - 2, 0x30 );
      pos = fillTo( pos, scrAt( row, 7 + HKW ), 0x30 );
      putStr( pos + 2, HelpTxt[i].desc, 44, 0x1F );
    }
    else if ( HelpTxt[i].desc ) {         /* a section: yellow title, a line to the frame */
      pos = putStr( scrAt( row, 7 ), HelpTxt[i].desc, 30, 0x1E );
      pos = putCh( pos, ' ', 0x1E );
      do pos = putCh( pos, (char)0xC4, 0x13 ); while ( pos < scrAt( row, 73 ) );
    }
  }
  if ( top ) putCh( scrAt( 1, 71 ), 0x18, 0x13 );         /* more above / below */
  if ( (int)NHELP - HROWS > top ) putCh( scrAt( 23, 71 ), 0x19, 0x13 );
  int pos = putCh( 24 * COLS, ' ', 0x70 );
  pos = putFooter( pos, FooterH );
  fillTo( pos, COLS * ROWS, 0x70 );
  mouseShow( );
}

/* F1: the keys in a table that scrolls (arrows, PgUp/PgDn, Home/End, the wheel); any
   other key or a click closes it. */
void showHelp( void ) {
  static const struct { unsigned char scan; signed char rows; } keys[] = {
    { 0x48, -1 }, { 0x50, 1 }, { 0x49, -HROWS }, { 0x51, HROWS }, { 0x47, -128 }, { 0x4F, 127 }
  };
  int top = 0;
  int max = (int)NHELP - HROWS;
  if ( max < 0 ) max = 0;
  for ( ;; ) {
    drawHelp( top );
    unsigned k = getInput( );
    int d;
    if ( k == K_WHEELUP ) d = -WSTEP;
    else if ( k == K_WHEELDN ) d = WSTEP;
    else {
      if ( ( k & 0xFF ) != 0 && ( k & 0xFF ) != 0xE0 ) return;
      int i;
      for ( i = 0; i < sizeof( keys ) / sizeof( keys[0] ) && keys[i].scan != ( k >> 8 ); i++ );
      if ( i == sizeof( keys ) / sizeof( keys[0] ) ) return;
      d = keys[i].rows;
    }
    top += d;
    if ( top > max ) top = max;
    if ( top < 0 ) top = 0;
  }
}
