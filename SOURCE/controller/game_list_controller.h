/*
   The game list: typing searches, Enter (or a click on the selected game) opens the game
   page, Ctrl+Enter (or a double click) plays, Ctrl+S runs the setup, Ctrl+E edits the
   METADATA.INI, F1 help, F2-F4 filters, F5 clears them, F6 / F7 favorite / favorites
   only, Shift+F7 without METADATA.INI only, F8 recently played, F9 sort order, F10 a
   random game. Esc clears the search, then the filters, then quits.
*/

#ifndef GMENU_CONTROLLER_GAME_LIST_CONTROLLER_H
#define GMENU_CONTROLLER_GAME_LIST_CONTROLLER_H

#include "list_runner.h"
#include "outcome.h"

class DetailController;
class GameLibrary;
class GameListView;
class HelpController;
class LaunchService;
class LibraryService;
class PickerController;
class Speaker;
struct FileList;
struct Session;
struct VisibleGames;

class GameListController : private ListPage {
public:
  GameListController( GameLibrary &library, Session &session, VisibleGames &visible, LibraryService &games,
                      LaunchService &launcher, GameListView &view, ListRunner &runner, DetailController &detail,
                      PickerController &picker, HelpController &help, Speaker &speaker, FileList &files );
  Outcome run();

private:
  void draw( const ListCursor &cursor );
  const char *label( int i );
  Outcome handleKey( unsigned key );
  Outcome handleText( unsigned char ch, int game );
  Outcome handleFunction( unsigned char scan, int game );
  Outcome handleEsc();
  Outcome play( int game );
  Outcome setup( int game );

  GameLibrary &library_;
  Session &session_;
  VisibleGames &visible_;
  LibraryService &games_;
  LaunchService &launcher_;
  GameListView &view_;
  ListRunner &runner_;
  DetailController &detail_;
  PickerController &picker_;
  HelpController &help_;
  Speaker &speaker_;
  FileList &files_;
  ListCursor cursor_;
};

#endif
