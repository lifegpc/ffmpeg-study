from avutil cimport AVRational, AVDictionary
from libc.stdint cimport int64_t


cdef extern from "libavformat/avformat.h":
    ctypedef struct AVChapter:
        int64_t id
        AVRational time_base
        int64_t start
        int64_t end
        AVDictionary *metadata
