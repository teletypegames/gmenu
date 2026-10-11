/*
   A picker: a box with values (the filters, the recently played games).
*/

#ifndef GMENU_VIEW_PICKER_VIEW_H
#define GMENU_VIEW_PICKER_VIEW_H

class Frame;
class ListCursor;
struct PickList;

struct PickerBox {
  int left, right; /* the columns of the box */
  const char *title;
};

class PickerView {
public:
  explicit PickerView( Frame &frame );
  void draw( const ListCursor &cursor, const PickList &picks, const PickerBox &box );

private:
  Frame &frame_;
};

#endif
