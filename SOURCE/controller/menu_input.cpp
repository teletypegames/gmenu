#include "menu_input.h"
#include "../lib/platform/clock.h"
#include "../lib/platform/idle.h"
#include "../lib/platform/keyboard.h"
#include "../lib/platform/mouse.h"
#include "../lib/ui/keys.h"

MenuInput::MenuInput( Keyboard &keyboard, Mouse &mouse, Idle &idle, Clock &clock )
  : keyboard_( keyboard ), mouse_( mouse ), idle_( idle ), clock_( clock ), row_( 0 ), col_( 0 ), buttons_( 0 ),
    double_( false ), lastClickTicks_( 0 ), lastClickRow_( -1 ) {}

int MenuInput::row() const {
  return row_;
}

int MenuInput::col() const {
  return col_;
}

bool MenuInput::doubleClick() const {
  return double_;
}

bool MenuInput::shiftDown() {
  return keyboard_.shiftDown();
}

unsigned MenuInput::next() {
  for ( ;; ) {
    idle_.idle();
    if ( keyboard_.ready() ) return keyboard_.read();
    unsigned code;
    if ( mouse_.present() && pollMouse( code ) ) return code;
  }
}

bool MenuInput::pollMouse( unsigned &code ) {
  MouseState state;
  mouse_.poll( state );
  row_ = state.row;
  col_ = state.col;
  unsigned char pressed = state.buttons & ~buttons_;
  buttons_ = state.buttons;
  if ( pressed & 1 ) {
    unsigned t = clock_.ticks();
    double_ = false;
    if ( t - lastClickTicks_ < DoubleTicks && row_ == lastClickRow_ ) {
      double_ = true;
      t -= DoubleTicks; /* a third click starts a new pair */
    }
    lastClickTicks_ = t;
    lastClickRow_ = row_;
    code = InputClick;
    return true;
  }
  if ( pressed & 2 ) {
    code = KeyEscCode;
    return true;
  }
  if ( mouse_.hasWheel() && state.wheel ) {
    code = state.wheel < 0 ? InputWheelUp : InputWheelDown;
    return true;
  }
  return false;
}

bool MenuInput::waitSecondClick() {
  mouse_.leftPresses();
  unsigned t = clock_.ticks();
  for ( ;; ) {
    if ( mouse_.leftPresses() ) break;
    if ( clock_.ticks() - t >= DoubleTicks ) return false;
  }
  lastClickRow_ = -1;
  MouseState state;
  mouse_.poll( state );
  buttons_ = state.buttons;
  return true;
}
