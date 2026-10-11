/*
   GMEDIT's screen: the form, the executables of the folder, and the hint of the active
   field (or an error) under them.
*/

#ifndef GMENU_VIEW_EDIT_FORM_VIEW_H
#define GMENU_VIEW_EDIT_FORM_VIEW_H

class Canvas;
class MetadataForm;
struct ExecutableList;

struct EditCursor {
  int field;  /* the active field */
  int pos;    /* the cursor in its value */
  int scroll; /* the first character shown */
};

class EditFormView {
public:
  explicit EditFormView( Canvas &canvas );
  /* The shown width of a field. */
  static int width( int field );
  void draw( MetadataForm &form, const char *folder, const ExecutableList &exes, const EditCursor &cursor,
             const char *message );

private:
  void drawField( MetadataForm &form, int field, const EditCursor &cursor );

  Canvas &canvas_;
};

#endif
