/*
   Starting a game: GMENURUN.BAT gets the command (GMENU.BAT runs it after the menu
   exited), the game gets one more start in the statistics and goes to the front of the
   recently played games.
*/

#ifndef GMENU_SERVICE_LAUNCH_SERVICE_H
#define GMENU_SERVICE_LAUNCH_SERVICE_H

class Clock;
class GameFilesRepository;
class GameLibrary;
class LauncherRepository;
class StatsRepository;
struct FileList;
struct Locations;
struct Session;

class LaunchService {
public:
  LaunchService( GameLibrary &library, Session &session, const Locations &where, LauncherRepository &launcher,
                 StatsRepository &stats, GameFilesRepository &files, Clock &clock );
  /* RUN, or the only executable of the folder; false when there is neither (the
     game page has to choose). */
  bool play( int game, FileList &files );
  /* SETUP; false when the game has none. */
  bool setup( int game );
  /* A file of the game's folder, with its arguments. */
  void runFile( int game, const char *file );
  /* GMEDIT.EXE for the game's METADATA.INI; false when it is missing. */
  bool editMetadata( int game );
  /* The executables (or all files) of the game's folder, sorted. */
  void listFiles( int game, bool all, FileList &files );
  /* The index of the RUN file in the list (0 when it is not there). */
  int runFileIndex( int game, const FileList &files );

private:
  void launch( int game, const char *command );

  GameLibrary &library_;
  Session &session_;
  const Locations &where_;
  LauncherRepository &launcher_;
  StatsRepository &stats_;
  GameFilesRepository &files_;
  Clock &clock_;
};

#endif
