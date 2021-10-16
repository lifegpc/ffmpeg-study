#ifndef _ENM4A_ENM4A_H
#define _ENM4A_ENM4A_H

#ifdef __cplusplus
extern "C" {
#endif
typedef enum ENM4A_ERROR {
    ENM4A_OK,
    ENM4A_NULL_POINTER,
    ENM4A_NO_MEMORY,
    ENM4A_FFMPEG_ERR,
    ENM4A_FILE_EXISTS,
    ENM4A_ERR_REMOVE_FILE,
    ENM4A_NO_AUDIO,
    ENM4A_ERR_OPEN_FILE,
} ENM4A_ERROR;

typedef enum ENM4A_LOG {
    ENM4A_LOG_INFO,
    ENM4A_LOG_VERBOSE,
    ENM4A_LOG_DEBUG,
    ENM4A_LOG_TRACE,
} ENM4A_LOG;

typedef enum ENM4A_OVERWRITE {
    ENM4A_OVERWRITE_ASK,
    ENM4A_OVERWRITE_YES,
    ENM4A_OVERWRITE_NO,
} ENM4A_OVERWRITE;

typedef struct ENM4A_ARGS {
    ENM4A_LOG level;
    ENM4A_OVERWRITE overwrite;
    char* output;
    char* title;
    char* artist;
    char* album;
    char* album_artist;
    char* cover;
    char* disc;
    char* track;
    char* date;
} ENM4A_ARGS;

ENM4A_ERROR encode_m4a(const char* input, ENM4A_ARGS args);
const char* enm4a_error_msg(ENM4A_ERROR err);
void enm4a_print_ffmpeg_version();
void enm4a_print_ffmpeg_configuration();
void enm4a_print_ffmpeg_license();
#ifdef __cplusplus
}
#endif

#endif
