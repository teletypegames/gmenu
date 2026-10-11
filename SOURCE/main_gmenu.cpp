/*
   GMENUPRG.EXE on DOS. The test build (build.sh test) takes its keys from TESTKEYS.TXT
   in the program's folder; when they run out it saves the screen to SCREEN.BIN there and
   exits with code 3.
*/

#include <direct.h>
#include <dos.h>
#include <stdlib.h>
#include <string.h>

#include "app/gmenu.h"
#include "app/machine.h"
#include "lib/platform/dos/bios_clock.h"
#include "lib/platform/dos/bios_keyboard.h"
#include "lib/platform/dos/dos_file_system.h"
#include "lib/platform/dos/dos_idle.h"
#include "lib/platform/dos/dos_screen.h"
#include "lib/platform/dos/dos_speaker.h"
#include "lib/platform/dos/dos_text_output.h"
#include "lib/platform/dos/int33_mouse.h"
#include "lib/testing/scripted_keyboard.h"
#include "lib/testing/screen_dump.h"
#include "model/locations.h"

#ifndef COLLECT
#define COLLECT "\\COLLECT" /* the default game directory (build.sh: gmenu.ini collect_dir) */
#endif

static Locations where;

static void locate( const char *argv0 ) {
  char cwd[80];
  unsigned drive;
  memset( &where, 0, sizeof( where ) );
  strcpy( where.gameDir, COLLECT );
  if ( getcwd( cwd, sizeof( cwd ) ) && strlen( cwd ) >= 3 && strlen( cwd ) - 3 < sizeof( where.startDir ) ) {
    strcpy( where.startDir, cwd + 3 );
  }
  where.setProgDir( argv0 );
  _dos_getdrive( &drive );
  where.homeDrive = where.gameDrive = (char)( 'A' + drive - 1 );
}

class DumpAndExit : public ScriptEnd {
public:
  DumpAndExit( Screen &screen, Mouse &mouse, FileSystem &fs ) : screen_( screen ), mouse_( mouse ), fs_( fs ) {}
  unsigned scriptEnded() {
    char path[96];
    mouse_.hide();
    where.progFile( "SCREEN.BIN", path );
    dumpScreen( screen_, fs_, path );
    exit( 3 );
    return 0;
  }

private:
  Screen &screen_;
  Mouse &mouse_;
  FileSystem &fs_;
};

int main( int argc, char *argv[] ) {
  static DosFileSystem fs;
  static DosScreen screen( DosScreen::SetMode );
  static Int33Mouse mouse;
  static BiosClock clock;
  static DosIdle idle;
  static DosSpeaker speaker;
  static DosTextOutput out;
  locate( argv[0] );
#ifdef TEST
  static char keys[4096];
  char path[96];
  where.progFile( "TESTKEYS.TXT", path );
  if ( readText( fs, path, keys, sizeof( keys ) ) < 0 ) keys[0] = 0;
  static DumpAndExit end( screen, mouse, fs );
  static ScriptedKeyboard keyboard( keys, end );
#else
  static BiosKeyboard keyboard;
#endif
  Machine machine = { fs, screen, keyboard, mouse, clock, idle, speaker, out };
  return runGmenu( machine, where, argc, argv );
}
