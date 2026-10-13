/*
   The scan of the game folders: what it is doing, in a box in the middle of the screen
   (the Progress of the scan).
*/

#ifndef GMENU_VIEW_SCAN_VIEW_H
#define GMENU_VIEW_SCAN_VIEW_H

#include "../lib/core/progress.h"

class Frame;

class ScanView : public Progress {
public:
  explicit ScanView( Frame &frame );
  void report( const char *text );

private:
  Frame &frame_;
};

#endif
