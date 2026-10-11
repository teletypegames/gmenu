#ifndef GMENU_REPOSITORY_SESSION_REPOSITORY_H
#define GMENU_REPOSITORY_SESSION_REPOSITORY_H

struct Session;

/* The session of the last run. */
class SessionRepository {
public:
  /* false when there is none (the session stays as it is) */
  virtual bool load( Session &session ) = 0;
  virtual void save( const Session &session ) = 0;
};

#endif
