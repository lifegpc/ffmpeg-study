#ifndef _RSSBOTLIB_ZIPFILE_H
#define _RSSBOTLIB_ZIPFILE_H
#include "memfile.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct ZipFile ZipFile;
ZipFile* open_zipfile(const char* fn);
MemFile* zipfile_get(ZipFile* s, const char* name);
void free_zipfile(ZipFile* f);
#ifdef __cplusplus
}
#endif
#endif
