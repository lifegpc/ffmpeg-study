#include "cstr_util.h"

#include <malloc.h>
#include <string.h>

int cstr_util_copy_str(char** dest, const char* str) {
    if (!dest || !str) return 1;
    size_t le = strlen(str);
    char* temp = malloc(le + 1);
    if (!temp) {
        return 2;
    }
    memcpy(temp, str, le);
    temp[le] = 0;
    dest = &temp;
    return 0;
}
