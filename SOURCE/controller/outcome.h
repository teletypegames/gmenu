#ifndef GMENU_CONTROLLER_OUTCOME_H
#define GMENU_CONTROLLER_OUTCOME_H

/* How a page ends: back to the page before, the menu exits (GMENURUN.BAT written:
   Launch, exit code 0; or Quit, exit code 1), or the game folders are scanned again
   (Rescan, then the menu goes on). */
enum Outcome { OutcomeBack, OutcomeLaunch, OutcomeQuit, OutcomeRescan };

#endif
