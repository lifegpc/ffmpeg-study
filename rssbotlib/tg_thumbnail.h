#ifndef _RSSBOTLIB_TG_THUMBNAIL_H
#define _RSSBOTLIB_TG_THUMBNAIL_H
#ifdef __cplusplus
extern "C" {
#endif
typedef enum TG_THUMBNAIL_ERROR_E {
    TG_THUMBNAIL_OK,
    TG_THUMBNAIL_FFMPEG_ERROR,
    TG_THUMBNAIL_NULL_POINTER,
    TG_THUMBNAIL_REMOVE_OUTPUT_FILE_FAILED,
    TG_THUMBNAIL_NO_VIDEO_STREAM,
    TG_THUMBNAIL_OOM,
    TG_THUMBNAIL_NO_DECODER,
    TG_THUMBNAIL_NO_ENCODER,
    TG_THUMBNAIL_UNABLE_SCALE,
} TG_THUMBNAIL_ERROR_E;

typedef struct TG_THUMBNAIL_ERROR {
    TG_THUMBNAIL_ERROR_E e;
    int fferr;
} TG_THUMBNAIL_ERROR;

typedef enum TG_THUMBNAIL_TYPE {
    TG_THUMBNAIL_JPEG,
    TG_THUMBNAIL_WEBP,
} TG_THUMBNAIL_TYPE;

typedef struct TG_THUMBNAIL_RESULT {
    TG_THUMBNAIL_ERROR err;
    int width;
    int height;
} TG_THUMBNAIL_RESULT;

void init_tg_thumbnail_error(TG_THUMBNAIL_ERROR* err);
const char* tg_thumbnail_error_msg(TG_THUMBNAIL_ERROR err);
TG_THUMBNAIL_RESULT convert_to_tg_thumbnail(const char* src, const char* dest, TG_THUMBNAIL_TYPE format);
#ifdef __cplusplus
}
#endif
#endif
