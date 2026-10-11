/*
   The command lines that start a game's files, and the GMENURUN.BAT that runs them:
   GMENU.BAT runs GMENURUN.BAT after the menu exited with code 0, then starts the menu
   again, so the menu is not in memory while a game runs.
*/

#ifndef GMENU_MODEL_LAUNCH_COMMAND_H
#define GMENU_MODEL_LAUNCH_COMMAND_H

#include "limits.h"

struct Game;
struct Locations;

enum { CommandSize = 2 * MaxCommand + 2, ScriptSize = 256 };

/* The first word of the command line is the file name (case-insensitive). */
bool commandRuns( const char *command, const char *file );
/* RUN with ARGS. */
void runCommand( const Game &game, char *out );
/* A file of the game's folder: the RUN file gets RUN's arguments and ARGS, the SETUP
   file SETUP's arguments. */
void fileCommand( const Game &game, const char *file, char *out );

/* To the game's folder, the command (a .BAT file with call), back to the folder of the
   start: the length. */
unsigned runScript( const Locations &where, const Game &game, const char *command, char *out );
/* GMEDIT.EXE for the game's folder: the length. */
unsigned editScript( const Locations &where, const Game &game, const char *editor, char *out );

#endif
