/*
   GMENUPRG.EXE - game menu for a game directory (DOS / DOSBox).
   Launch: GMENU.BAT -> GMENUPRG.EXE /B [/A] [game directory]   (default: \COLLECT)
           /A = all files are shown in a game's folder, not just the executables
   Exit code: 0 = GMENURUN.BAT written next to the program (GMENU.BAT runs it), 1 = quit,
   2 = error.
*/

#ifndef GMENU_APP_GMENU_H
#define GMENU_APP_GMENU_H

struct Locations;
struct Machine;

/* where: the program's folder, the start folder and drive, the default game directory. */
int runGmenu( Machine &machine, Locations &where, int argc, char *argv[] );

#endif
