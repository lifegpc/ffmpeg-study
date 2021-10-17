#include "cfileop.h"
#include "fileop.h"
#include <malloc.h>
#include <string.h>

int fileop_exists(char* fn) {
    if (!fn) return 0;
    return fileop::exists(fn) ? 1 : 0;
}

int fileop_remove(char* fn) {
    if (!fn) return 0;
    return fileop::remove(fn) ? 1 : 0;
}

char* fileop_dirname(const char* fn) {
    if (!fn) return NULL;
    auto re = fileop::dirname(fn);
    size_t le = re.length();
    char* tmp = (char*)malloc(le + 1);
    if (!tmp) return NULL;
    memcpy(tmp, re.c_str(), le);
    tmp[le] = 0;
    return tmp;
}
