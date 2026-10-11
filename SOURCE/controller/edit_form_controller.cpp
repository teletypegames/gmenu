#include <string.h>

#include "edit_form_controller.h"
#include "../lib/platform/keyboard.h"
#include "../lib/platform/speaker.h"
#include "../lib/ui/keys.h"
#include "../model/metadata_form.h"
#include "../service/metadata_edit_service.h"

static const char WriteFailed[] = "METADATA.INI could not be written (the disk is full or write-protected)";

EditFormController::EditFormController( MetadataEditService &editor, MetadataForm &form, EditFormView &view,
                                        Keyboard &keyboard, Speaker &speaker, const char *folder )
  : editor_( editor ), form_( form ), view_( view ), keyboard_( keyboard ), speaker_( speaker ), folder_( folder ),
    message_( NULL ) {}

void EditFormController::setField( int field ) {
  cursor_.field = field;
  cursor_.pos = strlen( form_.value( field ) );
  cursor_.scroll = 0;
}

void EditFormController::insert( char c ) {
  char *v = form_.value( cursor_.field );
  int len = strlen( v );
  if ( len >= form_.maxLength( cursor_.field ) ) {
    speaker_.beep();
    return;
  }
  memmove( v + cursor_.pos + 1, v + cursor_.pos, len - cursor_.pos + 1 );
  v[cursor_.pos++] = c;
}

void EditFormController::deleteAt( char *v, int pos ) {
  memmove( v + pos, v + pos + 1, strlen( v + pos ) );
}

void EditFormController::scrollToCursor() {
  int width = EditFormView::width( cursor_.field );
  if ( cursor_.pos < cursor_.scroll ) cursor_.scroll = cursor_.pos;
  if ( cursor_.pos - cursor_.scroll >= width ) cursor_.scroll = cursor_.pos - width + 1;
}

int EditFormController::run() {
  setField( 0 );
  for ( ;; ) {
    scrollToCursor();
    view_.draw( form_, folder_, editor_.executables(), cursor_, message_ );
    int code = handleKey( keyboard_.read() );
    if ( code != Continue ) return code;
  }
}

int EditFormController::save() {
  MetadataEditService::Save result = editor_.save();
  if ( result == MetadataEditService::Saved ) return 0;
  if ( result == MetadataEditService::Unchanged ) return 1;
  message_ = WriteFailed;
  speaker_.beep();
  return Continue;
}

int EditFormController::handleKey( unsigned key ) {
  unsigned char ch = keyAscii( key ), scan = keyScan( key );
  char *v = form_.value( cursor_.field );
  message_ = NULL;
  if ( ch == KeyEsc ) return 1;
  if ( ch == KeyTab || ch == KeyEnter ) setField( ( cursor_.field + 1 ) % FormFieldCount );
  else if ( ch == KeyBackspace ) {
    if ( cursor_.pos ) deleteAt( v, --cursor_.pos );
  }
  else if ( ch == KeyCtrlY ) {
    v[0] = 0;
    cursor_.pos = 0;
  }
  else if ( ch == 0 || ( ch == 0xE0 && scan ) ) return handleSpecial( scan, v );
  else if ( ch >= ' ' && ch != KeyDelChar ) insert( ch );
  return Continue;
}

int EditFormController::handleSpecial( unsigned char scan, char *v ) {
  switch ( scan ) {
    case ScanTab:
    case ScanUp: setField( ( cursor_.field + FormFieldCount - 1 ) % FormFieldCount ); break;
    case ScanDown: setField( ( cursor_.field + 1 ) % FormFieldCount ); break;
    case ScanLeft:
      if ( cursor_.pos ) cursor_.pos--;
      break;
    case ScanRight:
      if ( v[cursor_.pos] ) cursor_.pos++;
      break;
    case ScanHome: cursor_.pos = 0; break;
    case ScanEnd: setField( cursor_.field ); break;
    case ScanDel:
      if ( v[cursor_.pos] ) deleteAt( v, cursor_.pos );
      break;
    case ScanF2: return save();
  }
  return Continue;
}
