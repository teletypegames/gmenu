/*
   What GMENU keeps between runs (GMENU.SEL): the selected game, the search and the
   filters, the file list mode, the sort order and the recently played games.
*/

#ifndef GMENU_MODEL_SESSION_H
#define GMENU_MODEL_SESSION_H

#include "filter_criteria.h"
#include "recent_list.h"

struct Session {
  char selected[FolderSize];
  FilterCriteria criteria;
  char showAll; /* all files of a game, not just the executables */
  char sortOrder;
  RecentList recent;
};

#endif
