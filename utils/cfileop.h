#ifndef _UTIL_CFILEOP_H
#define _UTIL_CFILEOP_H
#ifdef __cplusplus
extern "C" {
#endif
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
#ifdef __cplusplus
}
#endif
#endif
