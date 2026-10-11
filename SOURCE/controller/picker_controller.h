/*
   The pickers: a filter's value (F2-F4), a recently played game (F8).
*/

#ifndef GMENU_CONTROLLER_PICKER_CONTROLLER_H
#define GMENU_CONTROLLER_PICKER_CONTROLLER_H

#include "list_runner.h"
#include "../model/pick_list.h"
#include "../view/picker_view.h"

class LibraryService;
class PickerService;
class Speaker;
struct Session;
struct VisibleGames;

class PickerController : private ListPage {
public:
  PickerController( PickerService &picker, LibraryService &library, Session &session, VisibleGames &visible,
                    PickerView &view, ListRunner &runner, Speaker &speaker );
  /* Sets the filter of the field from the values the games have. */
  void pickFilter( int field, const char *title );
  /* A recently played game into game (selected in the list too, when it is there);
     false when cancelled or there is none. */
  bool pickRecent( int &game );

private:
  bool run( int sel );
  void draw( const ListCursor &cursor );
  const char *label( int i );

  PickerService &picker_;
  LibraryService &library_;
  Session &session_;
  VisibleGames &visible_;
  PickerView &view_;
  ListRunner &runner_;
  Speaker &speaker_;
  PickList picks_;
  PickerBox box_;
  ListCursor cursor_;
};

#endif
