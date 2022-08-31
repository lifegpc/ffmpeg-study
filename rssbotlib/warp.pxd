from avcodec cimport AVCodecParameters

cdef extern from "warp.h":
    int get_codecpar_channels(AVCodecParameters* par)
