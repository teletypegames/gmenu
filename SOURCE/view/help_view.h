/*
   F1: the keys in a table that scrolls.
*/

#ifndef GMENU_VIEW_HELP_VIEW_H
#define GMENU_VIEW_HELP_VIEW_H

class Frame;

class HelpView {
public:
  explicit HelpView( Frame &frame );
  /* The rows of the table. */
  static int lineCount();
  void draw( int top );

private:
  Frame &frame_;
};

#endif
