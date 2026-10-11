/*
   GMEDIT.EXE on DOS. The test build (build.sh test) takes its keys from TESTKEYS.TXT in
   the program's folder; when they run out it saves the screen to SCREEN.BIN there and
   exits with code 3.
*/

#include <stdlib.h>
#include <string.h>

#include "app/gmedit.h"
#include "lib/platform/dos/bios_keyboard.h"
#include "lib/platform/dos/dos_file_system.h"
#include "lib/platform/dos/dos_screen.h"
#include "lib/platform/dos/dos_speaker.h"
#include "lib/platform/dos/dos_text_output.h"
#include "lib/testing/scripted_keyboard.h"
#include "lib/testing/screen_dump.h"
#include "model/locations.h"

static Locations where;

class DumpAndExit : public ScriptEnd {
public:
  DumpAndExit( Screen &screen, FileSystem &fs ) : screen_( screen ), fs_( fs ) {}
  unsigned scriptEnded() {
    char path[96];
    where.progFile( "SCREEN.BIN", path );
    dumpScreen( screen_, fs_, path );
    exit( 3 );
    return 0;
  }

private:
  Screen &screen_;
  FileSystem &fs_;
};

int main( int argc, char *argv[] ) {
  static DosFileSystem fs;
  static DosScreen screen( DosScreen::SetMode );
  static DosSpeaker speaker;
  static DosTextOutput out;
  where.setProgDir( argv[0] );
#ifdef TEST
  static char keys[4096];
  char path[96];
  where.progFile( "TESTKEYS.TXT", path );
  if ( readText( fs, path, keys, sizeof( keys ) ) < 0 ) keys[0] = 0;
  static DumpAndExit end( screen, fs );
  static ScriptedKeyboard keyboard( keys, end );
#else
  static BiosKeyboard keyboard;
#endif
  EditorMachine machine = { fs, screen, keyboard, speaker, out };
  return runGmedit( machine, argc, argv );
}
