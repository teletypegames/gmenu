/*
   F1: the help scrolls with the arrows, PgUp / PgDn, Home / End and the wheel; any other
   key or a click closes it.
*/

#ifndef GMENU_CONTROLLER_HELP_CONTROLLER_H
#define GMENU_CONTROLLER_HELP_CONTROLLER_H

class HelpView;
class MenuInput;

class HelpController {
public:
  HelpController( HelpView &view, MenuInput &input );
  void run();

private:
  /* false when the key closes the help */
  bool scrollOf( unsigned key, int &rows );

  HelpView &view_;
  MenuInput &input_;
};

#endif
