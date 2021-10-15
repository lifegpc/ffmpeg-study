#ifndef _ENM4A_ENM4A_H
#define _ENM4A_ENM4A_H

#ifdef __cplusplus
extern "C" {
#endif
typedef struct Enm4aNoInTotal Enm4aNoInTotal;

typedef enum ENM4A_ERROR {
    ENM4A_OK,
    ENM4A_NULL_POINTER,
    ENM4A_NO_MEMORY,
    ENM4A_INVALID_NO_IN_TOTAL,
    ENM4A_FFMPEG_ERR,
} ENM4A_ERROR;

typedef enum ENM4A_LOG {
    ENM4A_LOG_INFO,
    ENM4A_LOG_VERBOSE,
    ENM4A_LOG_DEBUG,
    ENM4A_LOG_TRACE,
} ENM4A_LOG;

typedef struct ENM4A_ARGS {
    ENM4A_LOG level;
    char* output;
    char* title;
    char* artist;
    char* album;
    char* album_artist;
    char* cover;
    Enm4aNoInTotal* disc;
    Enm4aNoInTotal* track;
} ENM4A_ARGS;

/**
 * @brief Parse no in total
 * @param dest Result
 * @param s Input string. such as 1/1.
 * @return ENM4A_OK if success.
*/
ENM4A_ERROR parse_no_in_total(Enm4aNoInTotal* dest, const char* s);
ENM4A_ERROR new_no_in_total(Enm4aNoInTotal* dest, size_t no, size_t total);
ENM4A_ERROR encode_m4a(const char* input, ENM4A_ARGS args);
const char* enm4a_error_msg(ENM4A_ERROR err);
#ifdef __cplusplus
}
#endif

#endif
