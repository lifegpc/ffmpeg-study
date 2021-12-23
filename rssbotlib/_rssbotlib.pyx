from avutil cimport *
from videoinfo cimport *
from avcodec cimport *
from ugoira cimport *
from ugoira cimport convert_ugoira_to_mp4 as cut4
from tg_thumbnail cimport *
from tg_thumbnail cimport convert_to_tg_thumbnail as cttt
from libc.string cimport memcpy
from libc.stdlib cimport malloc, free


def version():
    return [1, 0, 0, 2]


cdef inline void check_err(int re) except *:
    cdef char errbuf[AV_ERROR_MAX_STRING_SIZE]
    if re < 0:
        raise ValueError(try_decode(av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, re)))


cdef inline object try_decode(bytes b):
    try:
        return b.decode()
    except Exception:
        return b


cdef class AVDict:
    cdef AVDictionary* d
    def __cinit__(self):
        self.d = NULL

    def __dealloc__(self):
        av_dict_free(&self.d)

    def __delitem__(self, key):
        if isinstance(key, str):
            k = key.encode()
        elif isinstance(key, bytes):
            k = key
        else:
            k = str(key).encode()
        self.delete(k)

    def __getitem__(self, key):
        if isinstance(key, str):
            k = key.encode()
        elif isinstance(key, bytes):
            k = key
        else:
            k = str(key).encode()
        return self.get(k)

    def __len__(self):
        return av_dict_count(self.d)

    def __setitem__(self, key, value):
        if isinstance(key, str):
            k = key.encode()
        elif isinstance(key, bytes):
            k = key
        else:
            k = str(key).encode()
        if isinstance(value, str):
            v = value.encode()
        elif isinstance(value, bytes):
            v = value
        else:
            v = str(value).encode()
        self.set(k, v)

    def __str__(self):
        return str(self.to_list())

    cdef AVDictionary* cget(self):
        return self.d

    def copy(self, int flags = 0):
        n = AVDict()
        check_err(av_dict_copy(&n.d, self.d, flags))
        return n

    def delete(self, char* key, int flags = 0):
        check_err(av_dict_set(&self.d, key, NULL, flags))

    def get(self, char* key, int flags = 0):
        cdef AVDictionaryEntry* ent = av_dict_get(self.d, key, NULL, flags)
        if ent != NULL:
            return try_decode(ent.value)

    def set(self, char* key, char* value, int flags = 0):
        check_err(av_dict_set(&self.d, key, value, flags))

    def to_dict(self):
        d = {}
        if len(self) == 0:
            return d
        cdef int i = 0
        while i < self.d.count:
            d[try_decode(self.d.elems[i].key)] = try_decode(self.d.elems[i].value)
            i += 1
        return d

    def to_list(self):
        l = []
        if len(self) == 0:
            return l
        cdef int i = 0
        while i < self.d.count:
            l.append((try_decode(self.d.elems[i].key), try_decode(self.d.elems[i].value)))
            i += 1
        return l


cdef class Chapter:
    cdef AVChapter ch
    def __cinit__(self):
        init_avchapter(&self.ch)

    def __dealloc__(self):
        if self.ch.metadata != NULL:
            av_dict_free(&self.ch.metadata)

    @property
    def id(self):
        return self.ch.id

    @property
    def start(self):
        return self.ch.start * self.ch.time_base.num / self.ch.time_base.den

    @property
    def end(self):
        return self.ch.end * self.ch.time_base.num / self.ch.time_base.den

    @property
    def metadata(self):
        if self.ch.metadata != NULL:
            d = AVDict()
            check_err(av_dict_copy(&d.d, self.ch.metadata, 0))
            return d

    @property
    def title(self):
        cdef AVDictionaryEntry* ent = NULL
        if self.ch.metadata != NULL:
            ent = av_dict_get(self.ch.metadata, b'title', NULL, 0)
            if ent != NULL:
                return try_decode(ent.value)


cdef class StreamInfo:
    cdef CStreamInfo info
    def __cinit__(self):
        init_streaminfo(&self.info)

    def __dealloc__(self):
        free_streaminfo(&self.info)

    @property
    def bit_rate(self):
        if self.info.codecpar != NULL:
            return self.info.codecpar.bit_rate

    @property
    def channels(self):
        if self.info.codecpar != NULL and self.info.codecpar.codec_type == AVMEDIA_TYPE_AUDIO:
            return self.info.codecpar.channels

    @property
    def duration(self):
        if self.info.duration >= 0:
            return self.info.duration * self.info.time_base.num / self.info.time_base.den

    @property
    def height(self):
        if self.info.codecpar != NULL and self.info.codecpar.codec_type == AVMEDIA_TYPE_VIDEO:
            return self.info.codecpar.height

    @property
    def id(self):
        return self.info.id

    @property
    def index(self):
        return self.info.index

    @property
    def is_audio(self):
        return self.info.codecpar != NULL and self.info.codecpar.codec_type == AVMEDIA_TYPE_AUDIO

    @property
    def is_video(self):
        return self.info.codecpar != NULL and self.info.codecpar.codec_type == AVMEDIA_TYPE_VIDEO

    @property
    def metadata(self):
        if self.info.metadata != NULL:
            d = AVDict()
            check_err(av_dict_copy(&d.d, self.info.metadata, 0))
            return d

    @property
    def sample_aspect_ratio(self):
        if self.info.codecpar != NULL and self.info.codecpar.codec_type == AVMEDIA_TYPE_VIDEO:
            return self.info.codecpar.sample_aspect_ratio.num / self.info.codecpar.sample_aspect_ratio.den

    @property
    def sample_rate(self):
        if self.info.codecpar != NULL and self.info.codecpar.codec_type == AVMEDIA_TYPE_AUDIO:
            return self.info.codecpar.sample_rate

    @property
    def width(self):
        if self.info.codecpar != NULL and self.info.codecpar.codec_type == AVMEDIA_TYPE_VIDEO:
            return self.info.codecpar.width


cdef class VideoInfo:
    cdef CVideoInfo info
    def __cinit__(self):
        init_videoinfo(&self.info)

    def __dealloc__(self):
        free_videoinfo(&self.info)

    @property
    def chapters(self):
        l = []
        if self.info.nb_chapters == 0:
            return l
        cdef AVChapter* ch = NULL
        cdef unsigned int i = 0
        while i < self.info.nb_chapters:
            ch = self.info.chapters[i]
            c = Chapter()
            memcpy(&c.ch, ch, sizeof(AVChapter))
            c.ch.metadata = NULL;
            if ch.metadata != NULL:
                check_err(av_dict_copy(&c.ch.metadata, ch.metadata, 0))
            l.append(c)
            i += 1
        return l

    @property
    def duration(self):
        if self.info.duration > -1:
            return self.info.duration / AV_TIME_BASE

    @property
    def meta(self):
        if self.info.meta != NULL:
            d = AVDict()
            check_err(av_dict_copy(&d.d, self.info.meta, 0))
            return d

    @property
    def mime_type(self):
        if self.info.mime_type != NULL:
            return try_decode(self.info.mime_type)

    @property
    def nb_chapters(self):
        return self.info.nb_chapters

    @property
    def nb_streams(self):
        return self.info.nb_streams

    @property
    def ok(self):
        return True if self.info.ok else False

    def parse(self, s):
        if isinstance(s, str):
            k = s.encode()
        elif isinstance(s, bytes):
            k = s
        else:
            raise TypeError()
        cdef VideoInfoError err = parse_videoinfo(&self.info, k)
        if err.typ == VIDEOINFO_OK:
            return True
        elif err.typ == VIDEOINFO_FFMPEG_ERROR:
            print(try_decode(av_err2str(err.fferr)))
            return False
        else:
            print(try_decode(videoinfo_err_msg(err)))
            return False

    @property
    def streams(self):
        l = []
        if self.info.nb_streams == 0:
            return l
        cdef CStreamInfo* s = NULL
        cdef unsigned int i = 0
        while i < self.info.nb_streams:
            s = self.info.streams[i]
            si = StreamInfo()
            memcpy(&si.info, s, sizeof(CStreamInfo))
            si.info.metadata = NULL
            si.info.codecpar = NULL
            if s.metadata != NULL:
                check_err(av_dict_copy(&si.info.metadata, s.metadata, 0))
            if s.codecpar != NULL:
                si.info.codecpar = avcodec_parameters_alloc()
                if si.info.codecpar == NULL:
                    raise MemoryError()
                check_err(avcodec_parameters_copy(si.info.codecpar, s.codecpar))
            l.append(si)
            i += 1
        return l

    @property
    def type_long_name(self):
        if self.info.type_long_name != NULL:
            return try_decode(self.info.type_long_name)

    @property
    def type_name(self):
        if self.info.type_name != NULL:
            return try_decode(self.info.type_name)


def convert_ugoira_to_mp4(src, dest, frames, max_fps = 60.0, crf = None, opts: AVDict = None):
    if opts is not None:
        if not isinstance(opts, AVDict):
            raise TypeError('Not supported type.')
    cdef AVDictionary* opt = NULL if opts is None else opts.cget()
    cdef int tcrf = 0
    cdef int* dcrf = NULL
    cdef size_t nb_frames = len(frames)
    if nb_frames == 0:
        raise ValueError('frames is needed.')
    if crf is not None:
        tcrf = crf
        dcrf = &tcrf
    if isinstance(src, str):
        tsrc = src.encode()
    elif isinstance(src, bytes):
        tsrc = src
    else:
        tsrc = str(src).encode()
    if isinstance(dest, str):
        tdst = dest.encode()
    elif isinstance(dest, bytes):
        tdst = dest
    else:
        tdst = str(dest).encode()
    cdef UGOIRA_FRAME** cframes = <UGOIRA_FRAME**>malloc(sizeof(void*) * nb_frames)
    if cframes == NULL:
        raise MemoryError()
    cdef size_t i = 0
    cdef UGOIRA_ERROR err
    try:
        for o in frames:
            cframes[i] = new_ugoira_frame(o['file'].encode(), o['delay'])
            if cframes[i] == NULL:
                raise ValueError('Invalid frame: %s' % (o,))
            i += 1
        err = cut4(tsrc, tdst, cframes, nb_frames, max_fps, dcrf, opt)
    except Exception as e:
        for t in range(i):
            free_ugoira_frame(cframes[t])
        free(cframes)
        raise e
    i = 0
    while i < nb_frames:
        free_ugoira_frame(cframes[i])
        i += 1
    free(cframes)
    if err.e == UGOIRA_OK:
        return True
    elif err.e == UGOIRA_FFMPEG_ERROR:
        print(try_decode(av_err2str(err.fferr)))
        return False
    else:
        print(try_decode(ugoira_error_msg(err)))
        return False

def convert_to_tg_thumbnail(src, dest, format: str = 'webp'):
    cdef TG_THUMBNAIL_TYPE fmt = TG_THUMBNAIL_WEBP
    cdef TG_THUMBNAIL_RESULT re
    if isinstance(src, str):
        tsrc = src.encode()
    elif isinstance(src, bytes):
        tsrc = src
    else:
        tsrc = str(src).encode()
    if isinstance(dest, str):
        tdst = dest.encode()
    elif isinstance(dest, bytes):
        tdst = dest
    else:
        tdst = str(dest).encode()
    if format is not None:
        if format == 'webp':
            fmt = TG_THUMBNAIL_WEBP
        elif format == 'jpeg':
            fmt = TG_THUMBNAIL_JPEG
        else:
            raise ValueError('Unsupported output format.')
    re = cttt(tsrc, tdst, fmt)
    if re.err.e == TG_THUMBNAIL_OK:
        return (re.width, re.height)
    else:
        if re.err.e == TG_THUMBNAIL_FFMPEG_ERROR:
            msg = try_decode(av_err2str(re.err.fferr))
        else:
            msg = try_decode(tg_thumbnail_error_msg(re.err))
        raise ValueError(msg)
