#ifndef GMENU_REPOSITORY_LAUNCHER_REPOSITORY_H
#define GMENU_REPOSITORY_LAUNCHER_REPOSITORY_H

/* What GMENU.BAT runs after the menu: GMENURUN.BAT, and GMEDIT.EXE next to the menu. */
class LauncherRepository {
public:
  virtual void writeScript( const char *text, unsigned len ) = 0;
  /* GMEDIT.EXE's path into out; false when it is missing. */
  virtual bool findEditor( char *out ) = 0;
};

#endif
