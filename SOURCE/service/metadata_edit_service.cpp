#include <stddef.h>

#include "metadata_edit_service.h"
#include "../lib/core/text.h"

MetadataEditService::MetadataEditService( MetadataRepository &files, MetadataForm &form, const char *folder, char *raw,
                                          char *out )
  : files_( files ), form_( form ), folder_( folder ), raw_( raw ), out_( out ) {}

const ExecutableList &MetadataEditService::executables() const {
  return exes_;
}

MetadataRepository::Read MetadataEditService::load() {
  MetadataRepository::Read read = files_.readMetadata( raw_, MetadataForm::RawSize + 1 );
  if ( read == MetadataRepository::Loaded ) form_.load( raw_ );
  if ( read != MetadataRepository::Missing && read != MetadataRepository::Loaded ) return read;
  form_.keepAsOriginal();
  files_.listExecutables( exes_ );
  if ( read == MetadataRepository::Missing ) {
    char *text = out_;
    int n = files_.readNameFile( text, 4096 );
    const char *s = text;
    if ( n >= 0 ) {
      n = utf8To437( text );
      s = skipBom( text );
      n -= s - text;
    }
    form_.prefill( n >= 0 ? s : NULL, n, folder_, exes_ );
  }
  return read;
}

MetadataEditService::Save MetadataEditService::save() {
  if ( form_.isExisting() && !form_.isChanged() ) return Unchanged;
  unsigned len = form_.write( out_ );
  return files_.writeMetadata( out_, len ) ? Saved : Failed;
}
