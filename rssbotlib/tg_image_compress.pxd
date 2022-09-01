from avutil cimport AVDictionary

cdef extern from "tg_image_compress.h":
    ctypedef enum TG_IMAGE_ERROR_E:
        TG_IMAGE_OK
        TG_IMAGE_FFMPEG_ERROR
    ctypedef struct TG_IMAGE_ERROR:
        TG_IMAGE_ERROR_E e
        int fferr
    ctypedef enum TG_IMAGE_TYPE:
        TG_IMAGE_JPEG
        TG_IMAGE_PNG
    ctypedef struct TG_IMAGE_RESULT:
        TG_IMAGE_ERROR err
        int width
        int height
    void init_tg_image_error(TG_IMAGE_ERROR* err)
    const char* tg_image_error_msg(TG_IMAGE_ERROR err)
    TG_IMAGE_RESULT tg_image_compress(const char* src, const char* dest, TG_IMAGE_TYPE type, int max_len, AVDictionary* opts)
