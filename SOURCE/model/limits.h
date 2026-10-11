/*
   The limits of the METADATA.INI values, shared by GMENU and GMEDIT.
*/

#ifndef GMENU_MODEL_LIMITS_H
#define GMENU_MODEL_LIMITS_H

enum {
  MaxName = 50,
  MaxGenre = 24,
  MaxYear = 10,
  MaxPlayers = 10,
  MaxDesc = 2000,
  MaxCommand = 60, /* RUN, SETUP, ARGS */
  FolderSize = 13  /* an 8.3 name and its 0 */
};

static const char MetadataFile[] = "\\METADATA.INI";
static const char NameFile[] = "\\NAME.TXT";

#endif
