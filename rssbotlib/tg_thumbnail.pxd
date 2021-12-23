cdef extern from "tg_thumbnail.h":
    ctypedef enum TG_THUMBNAIL_ERROR_E:
        TG_THUMBNAIL_OK
        TG_THUMBNAIL_FFMPEG_ERROR
    ctypedef struct TG_THUMBNAIL_ERROR:
        TG_THUMBNAIL_ERROR_E e
        int fferr
    ctypedef enum TG_THUMBNAIL_TYPE:
        TG_THUMBNAIL_JPEG
        TG_THUMBNAIL_WEBP
    ctypedef struct TG_THUMBNAIL_RESULT:
        TG_THUMBNAIL_ERROR err
        int width
        int height
    void init_tg_thumbnail_error(TG_THUMBNAIL_ERROR* err)
    const char* tg_thumbnail_error_msg(TG_THUMBNAIL_ERROR err)
    TG_THUMBNAIL_RESULT convert_to_tg_thumbnail(const char* src, const char* dest, TG_THUMBNAIL_TYPE format)
