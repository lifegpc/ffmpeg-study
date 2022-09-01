#ifndef _RSSBOTLIB_TG_IMAGE_COMPRESS_H
#define _RSSBOTLIB_TG_IMAGE_COMPRESS_H
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/dict.h"
typedef enum TG_IMAGE_ERROR_E {
    TG_IMAGE_OK,
    TG_IMAGE_FFMPEG_ERROR,
    TG_IMAGE_NULL_POINTER,
    TG_IMAGE_REMOVE_OUTPUT_FILE_FAILED,
    TG_IMAGE_NO_VIDEO_STREAM,
    TG_IMAGE_OOM,
    TG_IMAGE_NO_DECODER,
    TG_IMAGE_NO_ENCODER,
    TG_IMAGE_UNABLE_SCALE,
} TG_IMAGE_ERROR_E;

typedef struct TG_IMAGE_ERROR {
    TG_IMAGE_ERROR_E e;
    int fferr;
} TG_IMAGE_ERROR;

typedef enum TG_IMAGE_TYPE {
    TG_IMAGE_JPEG,
    TG_IMAGE_PNG,
} TG_IMAGE_TYPE;

typedef struct TG_IMAGE_RESULT {
    TG_IMAGE_ERROR err;
    int width;
    int height;
} TG_IMAGE_RESULT;

void init_tg_image_error(TG_IMAGE_ERROR* err);
const char* tg_image_error_msg(TG_IMAGE_ERROR err);
TG_IMAGE_RESULT tg_image_compress(const char* src, const char* dest, TG_IMAGE_TYPE type, int max_len, AVDictionary* opts);
#ifdef __cplusplus
}
#endif
#endif
