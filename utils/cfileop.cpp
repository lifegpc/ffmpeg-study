#include "cfileop.h"
#include "fileop.h"
#include "cpp2c.h"

int fileop_exists(char* fn) {
    if (!fn) return 0;
    return fileop::exists(fn) ? 1 : 0;
}

int fileop_remove(char* fn) {
    if (!fn) return 0;
    return fileop::remove(fn) ? 1 : 0;
}

char* fileop_dirname(const char* fn) {
    if (!fn) return nullptr;
    auto re = fileop::dirname(fn);
    char* tmp = nullptr;
    return cpp2c::string2char(re, tmp) ? tmp : nullptr;
}

int fileop_is_url(const char* fn, int* re) {
    if (!fn || !re) return 0;
    *re = fileop::is_url(fn) ? 1 : 0;
    return 1;
}

char* fileop_basename(const char* fn) {
    if (!fn) return nullptr;
    auto re = fileop::basename(fn);
    char* tmp = nullptr;
    return cpp2c::string2char(re, tmp) ? tmp : nullptr;
}

int fileop_parse_size(const char* size, size_t* fs, int is_byte) {
    if (!size || !fs) return 0;
    size_t tmp;
    auto re = fileop::parse_size(size, tmp, is_byte);
    if (re) *fs = tmp;
    return re ? 1 : 0;
}
