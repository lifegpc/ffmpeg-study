from avutil cimport AVMediaType, AVCodecID, AVRational
from libc.stdint cimport int64_t


cdef extern from "libavcodec/codec_par.h":
    ctypedef struct AVCodecParameters:
        AVMediaType codec_type
        AVCodecID codec_id
        int format
        int64_t bit_rate
        int width
        int height
        AVRational sample_aspect_ratio
        int channels
        int sample_rate
    AVCodecParameters *avcodec_parameters_alloc()
    int avcodec_parameters_copy(AVCodecParameters *dst, const AVCodecParameters *src)
