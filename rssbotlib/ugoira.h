#ifndef _RSSBOTLIB_UGOIRA_H
#define _RSSBOTLIB_UGOIRA_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/dict.h"
typedef enum UGOIRA_ERROR_E {
    UGOIRA_OK,
    UGOIRA_FFMPEG_ERROR,
    UGOIRA_NULL_POINTER,
    UGOIRA_ZIP_OPEN_ERROR,
    UGOIRA_FILE_IN_ZIP_NOT_FOUND,
    UGOIRA_FRAMES_NEEDED,
    UGOIRA_NO_MEMORY,
    UGOIRA_REMOVE_OUTPUT_FILE_FAILED,
    UGOIRA_NO_AVAILABLE_ENCODER,
    UGOIRA_NO_VIDEO_STREAM,
    UGOIRA_NO_AVAILABLE_DECODER,
    UGOIRA_UNABLE_SCALE,
    UGOIRA_ERR_OPEN_FILE,
} UGOIRA_ERROR_E;

typedef struct UGOIRA_ERROR {
    UGOIRA_ERROR_E e;
    int fferr;
} UGOIRA_ERROR;

typedef struct UGOIRA_FRAME UGOIRA_FRAME;

UGOIRA_FRAME* new_ugoira_frame(const char* name, float delay);
void free_ugoira_frame(UGOIRA_FRAME* f);
void init_ugoira_error(UGOIRA_ERROR* err);
const char* ugoira_error_msg(UGOIRA_ERROR err);
UGOIRA_ERROR convert_ugoira_to_mp4(const char* src, const char* dest, UGOIRA_FRAME** frames, size_t nb_frames, float max_fps, int* crf, AVDictionary* opts);
#ifdef __cplusplus
}
#endif
#endif
