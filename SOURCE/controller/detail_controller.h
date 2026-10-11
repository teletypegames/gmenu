/*
   The game page: Enter (or a click) runs a file of the game's folder, Tab switches
   between its executables and all its files, Ctrl/Shift+PgUp / PgDn and the wheel over
   the left box scroll the description, F6 toggles the favorite, Ctrl+E edits the
   METADATA.INI, Esc goes back.
*/

#ifndef GMENU_CONTROLLER_DETAIL_CONTROLLER_H
#define GMENU_CONTROLLER_DETAIL_CONTROLLER_H

#include "list_runner.h"
#include "outcome.h"
#include "../model/file_list.h"

class DetailView;
class GameLibrary;
class HelpController;
class LaunchService;
class LibraryService;
class Speaker;
struct Session;

class DetailController : private ListPage {
public:
  DetailController( GameLibrary &library, Session &session, LibraryService &games, LaunchService &launcher,
                    DetailView &view, ListRunner &runner, HelpController &help, Speaker &speaker );
  Outcome run( int game );
  /* Ctrl+E on a game: GMEDIT.EXE runs after the menu (a beep without it). */
  Outcome editMetadata( int game );

private:
  void draw( const ListCursor &cursor );
  const char *label( int i );
  void rescan();
  /* The description scroll of the key; false when the key does not scroll. */
  bool scrollOf( unsigned key, int &rows );
  void scrollBy( int rows );

  GameLibrary &library_;
  Session &session_;
  LibraryService &games_;
  LaunchService &launcher_;
  DetailView &view_;
  ListRunner &runner_;
  HelpController &help_;
  Speaker &speaker_;
  FileList files_;
  ListCursor cursor_;
  int game_;
  int scroll_, lines_;
};

#endif
