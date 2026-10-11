/*
   What GMENU and GMEDIT get from the machine they run on (main_*.cpp builds it).
*/

#ifndef GMENU_APP_MACHINE_H
#define GMENU_APP_MACHINE_H

class Clock;
class FileSystem;
class Idle;
class Keyboard;
class Mouse;
class Screen;
class Speaker;
class TextOutput;

struct Machine {
  FileSystem &fs;
  Screen &screen;
  Keyboard &keyboard;
  Mouse &mouse;
  Clock &clock;
  Idle &idle;
  Speaker &speaker;
  TextOutput &out;
};

#endif
