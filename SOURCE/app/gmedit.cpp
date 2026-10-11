#include <stddef.h>

#include "gmedit.h"
#include "../controller/edit_form_controller.h"
#include "../lib/core/str.h"
#include "../lib/platform/keyboard.h"
#include "../lib/platform/screen.h"
#include "../lib/platform/text_output.h"
#include "../lib/ui/canvas.h"
#include "../model/metadata_form.h"
#include "../repository/files/folder_metadata_repository.h"
#include "../service/metadata_edit_service.h"
#include "../view/edit_form_view.h"

struct Options {
  bool fromMenu;   /* /B */
  char folder[66]; /* upper case, without a trailing '\' */
};

/* /B, and the first argument that is not a switch; false when there is none. */
static bool parseArgs( int argc, char *argv[], Options &options ) {
  options.fromMenu = false;
  for ( int i = 1; i < argc; i++ ) {
    const char *a = argv[i];
    if ( a[0] == '/' ) {
      if ( upcase( a[1] ) == 'B' ) options.fromMenu = true;
      continue;
    }
    int n = 0;
    for ( ; *a && n < 64; a++ ) options.folder[n++] = upcase( *a );
    while ( n > 1 && options.folder[n - 1] == '\\' ) n--;
    options.folder[n] = 0;
    return true;
  }
  return false;
}

/* An error, exit code 2; started by the menu, after a key. */
static int fail( EditorMachine &m, const Options &options, const char *message ) {
  m.out.write( message );
  if ( options.fromMenu ) {
    m.out.write( "Press a key." );
    m.keyboard.read();
  }
  return 2;
}

static const char *loadError( MetadataRepository::Read read ) {
  if ( read == MetadataRepository::Unreadable ) return "GMEDIT: METADATA.INI could not be read.\n";
  if ( read == MetadataRepository::TooBig ) return "GMEDIT: METADATA.INI is too big to edit (max 8 KB).\n";
  return NULL;
}

/* The editor put together, with its buffers. */
struct Gmedit {
  Gmedit( EditorMachine &m, const char *folder )
    : files( m.fs, folder ), editor( files, form, folder, raw, out ), canvas( m.screen ), view( canvas ),
      controller( editor, form, view, m.keyboard, m.speaker, folder ) {}

  MetadataForm form;
  char raw[MetadataForm::RawSize + 1];
  char out[MetadataForm::OutSize];
  FolderMetadataRepository files;
  MetadataEditService editor;
  Canvas canvas;
  EditFormView view;
  EditFormController controller;
};

static int edit( EditorMachine &m, Gmedit &app, const Options &options ) {
  if ( !app.files.folderExists() ) return fail( m, options, "GMEDIT: the game folder was not found.\n" );
  const char *error = loadError( app.editor.load() );
  if ( error ) return fail( m, options, error );
  m.screen.begin();
  int code = app.controller.run();
  m.screen.end();
  return code;
}

int runGmedit( EditorMachine &m, int argc, char *argv[] ) {
  Options options;
  if ( !parseArgs( argc, argv, options ) ) {
    return fail( m, options, "Usage: GMEDIT <game folder>   (creates or edits its METADATA.INI)\n" );
  }
  Gmedit *app = new Gmedit( m, options.folder );
  if ( app == NULL ) return fail( m, options, "GMEDIT: not enough memory.\n" );
  int code = edit( m, *app, options );
  delete app;
  return code;
}
