from avutil cimport AVDictionary, AVRational
from avcodec cimport AVCodecParameters
from avformat cimport AVChapter
from libc.stdint cimport int64_t


cdef extern from "videoinfo.h":
    ctypedef enum VideoInfoType:
        VIDEOINFO_OK
        VIDEOINFO_FFMPEG_ERROR
        VIDEOINFO_NULL_POINTER
        VIDEOINFO_NO_MEMORY
    ctypedef struct VideoInfoError:
        VideoInfoType typ
        int fferr
    ctypedef struct CStreamInfo:
        int index
        int id
        AVRational time_base
        int64_t duration
        AVDictionary* metadata
        AVCodecParameters* codecpar
    ctypedef struct CVideoInfo:
        char ok
        AVDictionary* meta
        int64_t duration
        const char* mime_type
        const char* type_name
        const char* type_long_name
        unsigned int nb_chapters
        AVChapter** chapters
        unsigned int nb_streams
        CStreamInfo** streams
    void init_avchapter(AVChapter* ch)
    void init_videoinfo(CVideoInfo* info)
    void init_streaminfo(CStreamInfo* info)
    VideoInfoError parse_videoinfo(CVideoInfo* info, char* url)
    void free_streaminfo(CStreamInfo* info)
    void free_videoinfo(CVideoInfo* info)
    void init_videoinfo_error(VideoInfoError* err)
    const char* videoinfo_err_msg(VideoInfoError err)
