/*
   The METADATA.INI of a game folder as GMEDIT edits it. A new file's form starts with
   what the folder tells: NAME from NAME.TXT (or the folder name), RUN if the folder has
   one executable besides the setup programs, SETUP from the first SETUP* / INSTALL*
   executable. An existing file is changed line by line: only the lines of the changed
   values are rewritten (a value's first line; its further lines, e.g. more DESCRIPTION=
   lines, are dropped), the other lines (comments, other keys, the values as they were, a
   byte order mark) are kept. A new value is written in UTF-8; an emptied value loses its
   line.
*/

#ifndef GMENU_MODEL_METADATA_FORM_H
#define GMENU_MODEL_METADATA_FORM_H

#include "limits.h"

struct ExecutableList;

enum FormField { FormName, FormGenre, FormYear, FormPlayers, FormDesc, FormRun, FormArgs, FormSetup, FormFieldCount };

class MetadataForm {
public:
  enum { RawSize = 8192, OutSize = 2 * RawSize + MaxDesc * 3 + 1024 };

  MetadataForm();
  char *value( int field );
  int maxLength( int field ) const;
  /* The file as it is (UTF-8, terminated); its values go into the form. */
  void load( const char *raw );
  bool isExisting() const;
  /* The values as they are now count as unchanged. */
  void keepAsOriginal();
  bool isChanged() const;
  /* A new file's values from NAME.TXT (text in CP437, len bytes), the folder name and the
     executables. */
  void prefill( const char *nameText, int len, const char *folder, const ExecutableList &exes );
  /* The new file into out: its length. */
  unsigned write( char *out ) const;

private:
  enum {
    ValuesSize = ( MaxName + 1 ) + ( MaxGenre + 1 ) + ( MaxYear + 1 ) + ( MaxPlayers + 1 ) + ( MaxDesc + 1 ) +
                 3 * ( MaxCommand + 1 )
  };

  void loadValue( int field, const char *v, int len );

  char values_[ValuesSize];
  char original_[ValuesSize];
  const char *raw_;
  char text_[4096];
};

#endif
