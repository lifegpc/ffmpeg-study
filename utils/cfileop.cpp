#include "cfileop.h"
#include "fileop.h"

int fileop_exists(char* fn) {
    if (!fn) return 0;
    return fileop::exists(fn) ? 1 : 0;
}

int fileop_remove(char* fn) {
    if (!fn) return 0;
    return fileop::remove(fn) ? 1 : 0;
}
