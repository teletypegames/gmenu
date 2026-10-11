/*
   GMEDIT's form: Tab / Enter / the arrows move between the fields, the editing keys
   change the value, Ctrl+Y clears it, F2 saves, Esc cancels. The result is the exit code:
   0 = METADATA.INI written, 1 = cancelled or nothing changed.
*/

#ifndef GMENU_CONTROLLER_EDIT_FORM_CONTROLLER_H
#define GMENU_CONTROLLER_EDIT_FORM_CONTROLLER_H

#include "../view/edit_form_view.h"

class EditFormView;
class Keyboard;
class MetadataEditService;
class MetadataForm;
class Speaker;

class EditFormController {
public:
  EditFormController( MetadataEditService &editor, MetadataForm &form, EditFormView &view, Keyboard &keyboard,
                      Speaker &speaker, const char *folder );
  int run();

private:
  enum { Continue = -1 };

  void setField( int field );
  void insert( char c );
  void deleteAt( char *v, int pos );
  int handleKey( unsigned key );
  int handleSpecial( unsigned char scan, char *v );
  int save();
  void scrollToCursor();

  MetadataEditService &editor_;
  MetadataForm &form_;
  EditFormView &view_;
  Keyboard &keyboard_;
  Speaker &speaker_;
  const char *folder_;
  EditCursor cursor_;
  const char *message_;
};

#endif
