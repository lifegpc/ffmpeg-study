#include "videoinfo.h"
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include "libavutil/error.h"

void init_avchapter(AVChapter* ch) {
    if (!ch) return;
    memset(ch, 0, sizeof(AVChapter));
    ch->time_base = AV_TIME_BASE_Q;
}

AVChapter* copy_chapter(AVChapter* chapter, VideoInfoError* err) {
    if (!chapter) {
        if (err) {
            err->typ = VIDEOINFO_NULL_POINTER;
        }
        return NULL;
    }
    AVChapter* ch = malloc(sizeof(AVChapter));
    if (!ch) {
        if (err) err->typ = VIDEOINFO_NO_MEMORY;
        return NULL;
    }
    memcpy(ch, chapter, sizeof(AVChapter));
    ch->metadata = NULL;
    int re = 0;
    if ((re = av_dict_copy(&ch->metadata, chapter->metadata, 0)) < 0) {
        if (err) {
            err->typ = VIDEOINFO_FFMPEG_ERROR;
            err->fferr = re;
        }
        free(ch);
        return NULL;
    }
    return ch;
}

CStreamInfo* copy_stream(AVStream* stream, VideoInfoError* err) {
    if (!stream) {
        if (err) err->typ = VIDEOINFO_NULL_POINTER;
        return NULL;
    }
    CStreamInfo* s = malloc(sizeof(CStreamInfo));
    if (!s) {
        if (err) err->typ = VIDEOINFO_NO_MEMORY;
        return NULL;
    }
    memset(s, 0, sizeof(CStreamInfo));
    s->index = stream->index;
    s->id = stream->id;
    s->time_base = stream->time_base;
    s->duration = stream->duration;
    int ret = 0;
    if (stream->metadata && (ret = av_dict_copy(&s->metadata, stream->metadata, 0)) < 0) {
        if (err) {
            err->typ = VIDEOINFO_FFMPEG_ERROR;
            err->fferr = ret;
        }
        free(s);
        return NULL;
    }
    s->codecpar = avcodec_parameters_alloc();
    if (!s->codecpar) {
        if (err) err->typ = VIDEOINFO_NO_MEMORY;
        av_dict_free(&s->metadata);
        free(s);
        return NULL;
    }
    if ((ret = avcodec_parameters_copy(s->codecpar, stream->codecpar)) < 0) {
        if (err) {
            err->typ = VIDEOINFO_FFMPEG_ERROR;
            err->fferr = ret;
        }
        avcodec_parameters_free(&s->codecpar);
        av_dict_free(&s->metadata);
        free(s);
        return NULL;
    }
    return s;
}

void free_chapter(AVChapter** ch) {
    if (!ch || !(*ch)) return;
    if ((*ch)->metadata) {
        av_dict_free(&(*ch)->metadata);
        (*ch)->metadata = NULL;
    }
    free(*ch);
    *ch = NULL;
}

VideoInfoError copy_chapters(AVChapter*** dst_chapters, unsigned int* dst_len, AVChapter** src_chapters, unsigned int src_len) {
    VideoInfoError re;
    init_videoinfo_error(&re);
    if (!dst_chapters || !dst_len) {
        re.typ = VIDEOINFO_NULL_POINTER;
        return re;
    }
    if (src_len == 0) {
        *dst_chapters = NULL;
        *dst_len = 0;
        return re;
    }
    if (!src_chapters) {
        re.typ = VIDEOINFO_NULL_POINTER;
        return re;
    }
    AVChapter** tmp = malloc(sizeof(void*) * src_len);
    if (!tmp) {
        re.typ = VIDEOINFO_NO_MEMORY;
        return re;
    }
    memset(tmp, 0, sizeof(void*) * src_len);
    for (unsigned int i = 0; i < src_len; i++) {
        AVChapter* ch = src_chapters[i];
        tmp[i] = copy_chapter(ch, &re);
        if (!tmp[i]) {
            goto freetmp;
        }
    }
    *dst_chapters = tmp;
    *dst_len = src_len;
    return re;
freetmp:
    for (unsigned int i = 0; i < src_len; i++) {
        AVChapter* ch = tmp[i];
        if (ch == NULL) {
            break;
        }
        free_chapter(&ch);
    }
    free(tmp);
    return re;
}

VideoInfoError copy_streams(CStreamInfo*** dst_streams, unsigned int* dst_len, AVStream** src_streams, unsigned int src_len) {
    VideoInfoError re;
    init_videoinfo_error(&re);
    if (!dst_streams || !dst_len) {
        re.typ = VIDEOINFO_NULL_POINTER;
        return re;
    }
    if (src_len == 0) {
        *dst_streams = NULL;
        *dst_len = 0;
        return re;
    }
    if (!src_streams) {
        re.typ = VIDEOINFO_NULL_POINTER;
        return re;
    }
    CStreamInfo** tmp = malloc(sizeof(void*) * src_len);
    if (!tmp) {
        re.typ = VIDEOINFO_NO_MEMORY;
        return re;
    }
    memset(tmp, 0, sizeof(void*) * src_len);
    for (unsigned int i = 0; i < src_len; i++) {
        AVStream* s = src_streams[i];
        tmp[i] = copy_stream(s, &re);
        if (!tmp[i]) {
            goto freetmp;
        }
    }
    *dst_streams = tmp;
    *dst_len = src_len;
    return re;
freetmp:
    for (unsigned int i = 0; i < src_len; i++) {
        CStreamInfo* s = tmp[i];
        if (s == NULL) break;
        free_streaminfo(s);
        free(s);
    }
    free(tmp);
    return re;
}

void init_videoinfo(CVideoInfo* info) {
    if (!info) return;
    memset(info, 0, sizeof(CVideoInfo));
    info->duration = -1;
}

void init_streaminfo(CStreamInfo* info) {
    if (!info) return;
    memset(info, 0, sizeof(CStreamInfo));
    info->duration = -1;
    info->time_base = AV_TIME_BASE_Q;
}

void free_streaminfo(CStreamInfo* info) {
    if (info->metadata) {
        av_dict_free(&info->metadata);
        info->metadata = NULL;
    }
    if (info->codecpar) {
        avcodec_parameters_free(&info->codecpar);
        info->codecpar = NULL;
    }
}

void free_chapters(AVChapter*** chapters, unsigned int* num) {
    if (!chapters || !num) {
        assert(0);
        return;
    }
    if (*num == 0) {
        *chapters = NULL;
        return;
    }
    for (unsigned int i = 0; i < *num; i++) {
        AVChapter* ch = (*chapters)[i];
        free_chapter(&ch);
    }
    free(*chapters);
    *chapters = NULL;
    *num = 0;
}

void free_streams(CStreamInfo*** streams, unsigned int* num) {
    if (!streams || !num) {
        assert(0);
        return;
    }
    if (*num == 0) {
        *streams = NULL;
        return;
    }
    for (unsigned int i = 0; i < *num; i++) {
        CStreamInfo* s = (*streams)[i];
        free_streaminfo(s);
        free(s);
    }
    free(*streams);
    *streams = NULL;
    *num = 0;
}

void free_videoinfo(CVideoInfo* info) {
    if (!info) return;
    if (info->meta) {
        av_dict_free(&info->meta);
        info->meta = NULL;
    }
    info->mime_type = NULL;
    info->type_name = NULL;
    info->type_long_name = NULL;
    free_chapters(&info->chapters, &info->nb_chapters);
    free_streams(&info->streams, &info->nb_streams);
}

void init_videoinfo_error(VideoInfoError* err) {
    if (!err) return;
    memset(err, 0, sizeof(VideoInfoError));
}

const char* videoinfo_err_msg(VideoInfoError err) {
    switch (err.typ) {
    case VIDEOINFO_OK:
        return "OK";
    case VIDEOINFO_NULL_POINTER:
        return "Arguments have null pointers.";
    case VIDEOINFO_FFMPEG_ERROR:
        return "A error occured in ffmpeg code.";
    case VIDEOINFO_NO_MEMORY:
        return "Out of memory.";
    default:
        return "Unknown error.";
    }
}

VideoInfoError parse_videoinfo(CVideoInfo* info, char* url) {
    VideoInfoError re;
    init_videoinfo_error(&re);
    if (!info || !url) {
        re.typ = VIDEOINFO_NULL_POINTER;
        return re;
    }
    if (info->ok) {
        free_videoinfo(info);
    }
    info->ok = 0;
    AVFormatContext* ic = NULL;
    if ((re.fferr = avformat_open_input(&ic, url, NULL, NULL)) < 0) {
        re.typ = VIDEOINFO_FFMPEG_ERROR;
        goto end;
    }
    if ((re.fferr = avformat_find_stream_info(ic, NULL)) < 0) {
        re.typ = VIDEOINFO_FFMPEG_ERROR;
        goto end;
    }
    info->ok = 1;
    info->duration = ic->duration;
    info->mime_type = ic->iformat->mime_type;
    info->type_name = ic->iformat->name;
    info->type_long_name = ic->iformat->long_name;
    if ((re.fferr = av_dict_copy(&info->meta, ic->metadata, 0)) < 0) {
        re.typ = VIDEOINFO_FFMPEG_ERROR;
        goto end;
    }
    re = copy_chapters(&info->chapters, &info->nb_chapters, ic->chapters, ic->nb_chapters);
    if (re.typ != VIDEOINFO_OK) {
        goto end;
    }
    re = copy_streams(&info->streams, &info->nb_streams, ic->streams, ic->nb_streams);
    if (re.typ != VIDEOINFO_OK) {
        goto end;
    }
end:
    if (ic) {
        avformat_close_input(&ic);
        ic = NULL;
    }
    return re;
}
