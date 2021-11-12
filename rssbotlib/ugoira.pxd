from avutil cimport AVDictionary
from libc.stddef cimport size_t 


cdef extern from "ugoira.h":
    ctypedef enum UGOIRA_ERROR_E:
        UGOIRA_OK
        UGOIRA_FFMPEG_ERROR
        UGOIRA_NULL_POINTER
        UGOIRA_ZIP_OPEN_ERROR
        UGOIRA_FILE_IN_ZIP_NOT_FOUND
    ctypedef struct UGOIRA_ERROR:
        UGOIRA_ERROR_E e
        int fferr
    ctypedef struct UGOIRA_FRAME:
        pass
    UGOIRA_FRAME* new_ugoira_frame(const char* name, float delay)
    void free_ugoira_frame(UGOIRA_FRAME* f)
    void init_ugoira_error(UGOIRA_ERROR* err)
    const char* ugoira_error_msg(UGOIRA_ERROR err)
    UGOIRA_ERROR convert_ugoira_to_mp4(const char* src, const char* dest, UGOIRA_FRAME** frames, size_t nb_frames, float max_fps, int* crf, AVDictionary* opts)
