/*
   The input of the menu: BIOS key codes, and the mouse as extra codes. A left click is
   Click (its cell in row() / col(), a double click when doubleClick()), a right click is
   Esc, the wheel is WheelUp / WheelDown.
*/

#ifndef GMENU_CONTROLLER_MENU_INPUT_H
#define GMENU_CONTROLLER_MENU_INPUT_H

class Clock;
class Idle;
class Keyboard;
class Mouse;

enum {
  InputClick = 0xFF01,
  InputWheelUp = 0xFF02,
  InputWheelDown = 0xFF03,
  InputDoubleClick = 0xFF04 /* the list runner's: a double click in the game list */
};

class MenuInput {
public:
  enum { WheelStep = 3 }; /* items per notch */

  MenuInput( Keyboard &keyboard, Mouse &mouse, Idle &idle, Clock &clock );
  unsigned next();
  int row() const;
  int col() const;
  bool doubleClick() const;
  /* After a click on the selected item: true when a second click follows in time. */
  bool waitSecondClick();
  bool shiftDown();

private:
  enum { DoubleTicks = 9 }; /* the most timer ticks between the clicks */

  bool pollMouse( unsigned &code );

  Keyboard &keyboard_;
  Mouse &mouse_;
  Idle &idle_;
  Clock &clock_;
  int row_, col_;
  unsigned char buttons_;
  bool double_;
  unsigned lastClickTicks_;
  int lastClickRow_;
};

#endif
