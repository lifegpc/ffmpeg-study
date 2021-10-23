#ifndef _UTIL_CFILEOP_H
#define _UTIL_CFILEOP_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
/**
 * @brief Check file exists
 * @param fn File name
 * @returns 1 if file is exists otherwise 0
*/
int fileop_exists(char* fn);
/**
 * @brief Remove file
 * @param fn File name
 * @returns 1 if successed otherwise 0
*/
int fileop_remove(char* fn);
/**
 * @brief Get directory name from path
 * @param fn Path
 * @returns Directory. If path does not contain a slash, will return "". Need free memory by using free.
*/
char* fileop_dirname(const char* fn);
/**
 * @brief Detect if path is a url.
 * @param fn Path
 * @param re result
 * @return 1 if successed otherwise 0
*/
int fileop_is_url(const char* fn, int* re);
/**
 * @brief Get file name from path
 * @param fn Path
 * @return File name. Need free memory by calling free.
*/
char* fileop_basename(const char* fn);
/**
 * @brief Parse size string
 * @param size Such as "10KiB", "10", "34B", "48K"
 * @param fs size
 * @param is_byte Whether to return bits or bytes
 * @return 1 if successed otherwise 0
*/
int fileop_parse_size(const char* size, size_t* fs, int is_byte);
#ifdef __cplusplus
}
#endif
#endif
