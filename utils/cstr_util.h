#ifndef _UTILS_CSTR_UTIL_H
#define _UTILS_CSTR_UTIL_H
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Copy string to another string
 * @param dest The pointer of output string
 * @param str The input string
 * @return 0 if successed. `1` - args contains NULL. `2` - Out of memory.
*/
int cstr_util_copy_str(char** dest, const char* str);
#ifdef __cplusplus
}
#endif
#endif
