#ifndef GMENU_CONTROLLER_OUTCOME_H
#define GMENU_CONTROLLER_OUTCOME_H

/* How a page ends: back to the page before, or the menu exits (GMENURUN.BAT written:
   Launch, exit code 0; or Quit, exit code 1). */
enum Outcome { OutcomeBack, OutcomeLaunch, OutcomeQuit };

#endif
