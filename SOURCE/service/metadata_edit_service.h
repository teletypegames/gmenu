/*
   Creating or editing a game folder's METADATA.INI (see MetadataForm).
*/

#ifndef GMENU_SERVICE_METADATA_EDIT_SERVICE_H
#define GMENU_SERVICE_METADATA_EDIT_SERVICE_H

#include "../model/executable_list.h"
#include "../model/metadata_form.h"
#include "../repository/metadata_repository.h"

class MetadataEditService {
public:
  enum Save { Saved, Unchanged, Failed };

  /* raw: MetadataForm::RawSize + 1 bytes, out: MetadataForm::OutSize bytes */
  MetadataEditService( MetadataRepository &files, MetadataForm &form, const char *folder, char *raw, char *out );
  /* The form from the folder: Missing (a new file), Loaded, or why it cannot be edited. */
  MetadataRepository::Read load();
  Save save();
  const ExecutableList &executables() const;

private:
  MetadataRepository &files_;
  MetadataForm &form_;
  const char *folder_;
  ExecutableList exes_;
  char *raw_;
  char *out_;
};

#endif
