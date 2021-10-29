cdef extern from "libavutil/avutil.h":
    const int AV_TIME_BASE
    ctypedef enum AVMediaType:
        AVMEDIA_TYPE_UNKNOWN = -1
        AVMEDIA_TYPE_VIDEO
        AVMEDIA_TYPE_AUDIO
        AVMEDIA_TYPE_DATA
        AVMEDIA_TYPE_SUBTITLE
        AVMEDIA_TYPE_ATTACHMENT
        AVMEDIA_TYPE_NB
    ctypedef enum AVCodecID:
        pass


cdef extern from "_avutil_dict.h":
    ctypedef struct AVDictionaryEntry:
        char* key
        char* value
    ctypedef struct AVDictionary:
        int count
        AVDictionaryEntry *elems
    AVDictionaryEntry *av_dict_get(const AVDictionary *m, const char *key, const AVDictionaryEntry *prev, int flags)
    int av_dict_count(const AVDictionary *m)
    int av_dict_copy(AVDictionary **dst, const AVDictionary *src, int flags)
    int av_dict_set(AVDictionary **pm, const char *key, const char *value, int flags)
    void av_dict_free(AVDictionary **m)


cdef extern from "libavutil/error.h":
    const int AV_ERROR_MAX_STRING_SIZE
    int av_strerror(int errnum, char *errbuf, size_t errbuf_size)
    char *av_make_error_string(char *errbuf, size_t errbuf_size, int errnum)
    char *av_err2str(int errnum)


cdef extern from "libavutil/rational.h":
    ctypedef struct AVRational:
        int num
        int den
