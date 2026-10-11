/*
   GMEDIT.EXE - creates or edits the METADATA.INI of a game folder (DOS / DOSBox).
   Launch: GMEDIT.EXE [/B] <game folder>    e.g. GMEDIT C:\COLLECT\DOOM
           GMENUPRG.EXE starts it through GMENURUN.BAT (Ctrl+E) with /B: an error message
           then waits for a key, otherwise the menu would clear it at once.
   Exit code: 0 = METADATA.INI written, 1 = cancelled or nothing changed, 2 = error.
*/

#ifndef GMENU_APP_GMEDIT_H
#define GMENU_APP_GMEDIT_H

class FileSystem;
class Keyboard;
class Screen;
class Speaker;
class TextOutput;

/* What GMEDIT gets from the machine it runs on (main_gmedit.cpp builds it). */
struct EditorMachine {
  FileSystem &fs;
  Screen &screen;
  Keyboard &keyboard;
  Speaker &speaker;
  TextOutput &out;
};

int runGmedit( EditorMachine &machine, int argc, char *argv[] );

#endif
