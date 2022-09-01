#include "ugoira.h"
#include <string.h>
#include "zipfile.h"
#include <malloc.h>
#include "cfileop.h"
#include "cmath.h"
#include <stdint.h>

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
#include "libavutil/timestamp.h"
#include "utils.h"

typedef struct UGOIRA_FRAME {
    char* file;
    float delay;
} UGOIRA_FRAME;

UGOIRA_FRAME* new_ugoira_frame(const char* name, float delay) {
    if (!name || delay < 1) return NULL;
    UGOIRA_FRAME* f = malloc(sizeof(UGOIRA_FRAME));
    if (!f) return NULL;
    size_t le = strlen(name);
    f->file = malloc(le + 1);
    if (!f->file) {
        free(f);
        return NULL;
    }
    memcpy(f->file, name, le);
    f->file[le] = 0;
    f->delay = delay;
    return f;
}

void free_ugoira_frame(UGOIRA_FRAME* f) {
    if (!f) return;
    if (f->file) {
        free(f->file);
        f->file = NULL;
    }
    free(f);
}

void init_ugoira_error(UGOIRA_ERROR* err) {
    if (!err) return;
    memset(err, 0, sizeof(UGOIRA_ERROR));
}

float ugoira_cal_fps(UGOIRA_FRAME** frames, size_t nb_frames, float max_fps) {
    if (nb_frames == 0) return max_fps;
    int re = frames[0]->delay;
    size_t i = 1;
    for (; i < nb_frames; i++) {
        re = GCD(re, frames[i]->delay);
    }
    return FFMIN(1000 / ((float)re), max_fps);
}

const char* ugoira_error_msg(UGOIRA_ERROR err) {
    switch (err.e) {
    case UGOIRA_OK:
        return "OK";
    case UGOIRA_FFMPEG_ERROR:
        return "A error occured in ffmpeg code.";
    case UGOIRA_NULL_POINTER:
        return "Arguments have null pointers.";
    case UGOIRA_ZIP_OPEN_ERROR:
        return "Can not open zip file.";
    case UGOIRA_FILE_IN_ZIP_NOT_FOUND:
        return "Can not find a file in zip file.";
    case UGOIRA_FRAMES_NEEDED:
        return "frames is needed.";
    case UGOIRA_NO_MEMORY:
        return "Out of memory.";
    case UGOIRA_REMOVE_OUTPUT_FILE_FAILED:
        return "Can not remove output file.";
    case UGOIRA_NO_AVAILABLE_ENCODER:
        return "No available encoder.";
    case UGOIRA_NO_VIDEO_STREAM:
        return "No video stream available in the file.";
    case UGOIRA_NO_AVAILABLE_DECODER:
        return "No available decoder.";
    case UGOIRA_UNABLE_SCALE:
        return "Unable to scale image.";
    case UGOIRA_ERR_OPEN_FILE:
        return "Unable to open output file.";
    default:
        return "Unknown error.";
    }
}

const AVCodec* ugoira_find_encoder() {
    const AVCodec* c = avcodec_find_encoder_by_name("libx264");
    if (!c) c = avcodec_find_encoder(AV_CODEC_ID_H264);
    return c;
}

UGOIRA_ERROR_E ugoira_encode_video(UGOIRA_ERROR* err, AVFrame* ofr, AVFormatContext* oc, AVCodecContext* eoc, char* writed_data, int64_t* pts, unsigned int stream_index, AVRational time_base) {
    if (!oc || !eoc || !writed_data) {
        if (err) err->e = UGOIRA_NULL_POINTER;
        return UGOIRA_NULL_POINTER;
    }
    if (ofr && !pts) {
        if (err) err->e = UGOIRA_NULL_POINTER;
        return UGOIRA_NULL_POINTER;
    }
    UGOIRA_ERROR_E re = UGOIRA_OK;
    *writed_data = 0;
    int ret = 0;
    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        re = UGOIRA_NO_MEMORY;
        goto end;
    }
    if (ofr) {
        ofr->pts = *pts;
        *pts += av_rescale_q_rnd(1, time_base, oc->streams[stream_index]->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        ofr->pkt_dts = ofr->pts;
    }
    if ((ret = avcodec_send_frame(eoc, ofr)) < 0) {
        if (ret == AVERROR_EOF) {
            ret = 0;
        } else {
            re = UGOIRA_FFMPEG_ERROR;
            goto end;
        }
    }
    ret = avcodec_receive_packet(eoc, pkt);
    if (ret >= 0) {
        *writed_data = 1;
    } else if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
        ret = 0;
        goto end;
    } else {
        re = UGOIRA_FFMPEG_ERROR;
        goto end;
    }
    if (*writed_data && pkt) {
        pkt->stream_index = stream_index;
        if ((ret = av_write_frame(oc, pkt)) < 0) {
            re = UGOIRA_FFMPEG_ERROR;
            goto end;
        }
    }
end:
    if (pkt) av_packet_free(&pkt);
    if (err) {
        err->e = re;
        if (re == UGOIRA_FFMPEG_ERROR) {
            err->fferr = ret;
        }
    }
    return re;
}

UGOIRA_ERROR convert_ugoira_to_mp4(const char* src, const char* dest, UGOIRA_FRAME** frames, size_t nb_frames, float max_fps, int* crf, AVDictionary* opts) {
    UGOIRA_ERROR re;
    init_ugoira_error(&re);
    if (!src || !dest || !frames) {
        re.e = UGOIRA_NULL_POINTER;
        return re;
    }
    if (!nb_frames) {
        re.e = UGOIRA_FRAMES_NEEDED;
        return re;
    }
    size_t i = 0;
    for (; i < nb_frames; i++) {
        UGOIRA_FRAME* frame = frames[i];
        if (!frame) {
            re.e = UGOIRA_NULL_POINTER;
            return re;
        }
    }
    int dcrf = 18;
    if (crf && *crf >= -1) {
        dcrf = *crf;
    }
    AVRational fps = { (int)(ugoira_cal_fps(frames, nb_frames, max_fps) * AV_TIME_BASE + 0.5), AV_TIME_BASE };
    AVRational time_base = { fps.den, fps.num };
    AVFormatContext* ic = NULL, * oc = NULL;
    AVIOContext* iioc = NULL;
    AVDictionary* opt = NULL;
    ZipFile* sf = NULL;
    MemFile* mf = NULL;
    unsigned char* buff = NULL;
    AVCodecContext* eoc = NULL, * eic = NULL;
    AVStream* is = NULL, * os = NULL;
    const AVCodec* output_codec = NULL, * input_codec = NULL;
    struct SwsContext* sws_ctx = NULL;
    enum AVPixelFormat pre_pixfmt = AV_PIX_FMT_NONE;
    int pre_width = -1, pre_height = -1;
    int64_t pts = 0, max_de = 0;
    AVPacket pkt;
    AVFrame* ifr = NULL, * ofr = NULL;
    char writed = 0;
    const static AVRational tb = { 1, 1000 };
    if (fileop_exists(dest)) {
        if (!fileop_remove(dest)) {
            re.e = UGOIRA_REMOVE_OUTPUT_FILE_FAILED;
            goto end;
        }
    }
    if (!(ifr = av_frame_alloc())) {
        re.e = UGOIRA_NO_MEMORY;
        goto end;
    }
    if (!(ofr = av_frame_alloc())) {
        re.e = UGOIRA_NO_MEMORY;
        goto end;
    }
    sf = open_zipfile(src);
    if (!sf) {
        re.e = UGOIRA_ZIP_OPEN_ERROR;
        goto end;
    }
    if (opts) {
        if ((re.fferr = av_dict_copy(&opt, opts, 0)) < 0) {
            re.e = UGOIRA_FFMPEG_ERROR;
            goto end;
        }
    }
    for (i = 0; i < nb_frames; i++) {
        UGOIRA_FRAME* frame = frames[i];
        if (i != 0) {
            if (eic) {
                avcodec_free_context(&eic);
                eic = NULL;
            }
            avformat_close_input(&ic);
            ic = NULL;
            free_memfile(mf);
            mf = NULL;
            if (iioc) av_freep(&iioc->buffer);
            avio_context_free(&iioc);
            iioc = NULL;
        }
        if (!(ic = avformat_alloc_context())) {
            re.e = UGOIRA_NO_MEMORY;
            goto end;
        }
        if (!(mf = zipfile_get(sf, frame->file))) {
            re.e = UGOIRA_FILE_IN_ZIP_NOT_FOUND;
            goto end;
        }
        buff = av_malloc(4096);
        if (!buff) {
            re.e = UGOIRA_NO_MEMORY;
            goto end;
        }
        if (!(iioc = avio_alloc_context(buff, 4096, 0, (void*)mf, &memfile_readpacket, NULL, NULL))) {
            re.e = UGOIRA_NO_MEMORY;
            goto end;
        }
        ic->pb = iioc;
        if ((re.fferr = avformat_open_input(&ic, NULL, NULL, NULL)) < 0) {
            re.e = UGOIRA_FFMPEG_ERROR;
            goto end;
        }
        if ((re.fferr = avformat_find_stream_info(ic, NULL)) < 0) {
            re.e = UGOIRA_FFMPEG_ERROR;
            goto end;
        }
        av_dump_format(ic, i, frame->file, 0);
        for (unsigned int si = 0; si < ic->nb_streams; i++) {
            AVStream* s = ic->streams[si];
            if (s->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                is = s;
                break;
            }
        }
        if (!is) {
            re.e = UGOIRA_NO_VIDEO_STREAM;
            goto end;
        }
        if (!(input_codec = avcodec_find_decoder(is->codecpar->codec_id))) {
            re.e = UGOIRA_NO_AVAILABLE_DECODER;
            goto end;
        }
        if (!(eic = avcodec_alloc_context3(input_codec))) {
            re.e = UGOIRA_NO_MEMORY;
            goto end;
        }
        if ((re.fferr = avcodec_parameters_to_context(eic, is->codecpar)) < 0) {
            re.e = UGOIRA_FFMPEG_ERROR;
            goto end;
        }
        if ((re.fferr = avcodec_open2(eic, input_codec, NULL)) < 0) {
            re.e = UGOIRA_FFMPEG_ERROR;
            goto end;
        }
        if (i == 0) {
            output_codec = ugoira_find_encoder();
            if (!output_codec) {
                re.e = UGOIRA_NO_AVAILABLE_ENCODER;
                goto end;
            }
            if ((re.fferr = avformat_alloc_output_context2(&oc, NULL, "mp4", dest)) < 0) {
                re.e = UGOIRA_FFMPEG_ERROR;
                goto end;
            }
            if (!(eoc = avcodec_alloc_context3(output_codec))) {
                re.e = UGOIRA_NO_MEMORY;
                goto end;
            }
            eoc->width = eic->width;
            eoc->height = eic->height;
            eoc->sample_aspect_ratio = eic->sample_aspect_ratio;
            eoc->framerate = fps;
            AVDictionaryEntry* force_yuv420p = NULL;
            if (opt) {
                force_yuv420p = av_dict_get(opt, "force_yuv420p", NULL, 0);
            }
            if (!force_yuv420p && is_supported_pixfmt(eic->pix_fmt, output_codec->pix_fmts)) {
                eoc->pix_fmt = eic->pix_fmt;
            } else {
                eoc->pix_fmt = AV_PIX_FMT_YUV420P;
            }
            ofr->width = eoc->width;
            ofr->height = eoc->height;
            ofr->format = eoc->pix_fmt;
            eoc->time_base = AV_TIME_BASE_Q;
            if (!strcmp(output_codec->name, "libx264")) {
                AVDictionaryEntry* tmp = NULL;
                if (opt) {
                    tmp = av_dict_get(opt, "preset", NULL, 0);
                }
                if (tmp) {
                    av_opt_set(eoc->priv_data, "preset", tmp->value, 0);
                } else {
                    av_opt_set(eoc->priv_data, "preset", "slow", 0);
                }
                av_opt_set_int(eoc->priv_data, "crf", dcrf, 0);
                if (opt) {
                    tmp = av_dict_get(opt, "level", NULL, 0);
                }
                if (tmp) {
                    av_opt_set(eoc->priv_data, "level", tmp->value, 0);
                }
                if (opt) {
                    tmp = av_dict_get(opt, "profile", NULL, 0);
                }
                if (tmp) {
                    av_opt_set(eoc->priv_data, "profile", tmp->value, 0);
                }
            }
            if ((re.fferr = av_frame_get_buffer(ofr, 0)) < 0) {
                re.e = UGOIRA_FFMPEG_ERROR;
                goto end;
            }
            if (!(os = avformat_new_stream(oc, output_codec))) {
                re.e = UGOIRA_NO_MEMORY;
                goto end;
            }
            os->avg_frame_rate = fps;
            os->r_frame_rate = fps;
            os->time_base = AV_TIME_BASE_Q;
            if ((re.fferr = avcodec_open2(eoc, output_codec, NULL)) < 0) {
                re.e = UGOIRA_FFMPEG_ERROR;
                goto end;
            }
            if ((re.fferr = avcodec_parameters_from_context(os->codecpar, eoc)) < 0) {
                re.e = UGOIRA_FFMPEG_ERROR;
                goto end;
            }
            av_dump_format(oc, 0, dest, 1);
            if (!(oc->oformat->flags & AVFMT_NOFILE)) {
                int ret = avio_open(&oc->pb, dest, AVIO_FLAG_WRITE);
                if (ret < 0) {
                    re.e = UGOIRA_ERR_OPEN_FILE;
                    goto end;
                }
            }
            if ((re.fferr = avformat_write_header(oc, NULL)) < 0) {
                re.e = UGOIRA_FFMPEG_ERROR;
                goto end;
            }
        }
        if (!sws_ctx || eic->pix_fmt != pre_pixfmt || eic->width != pre_width || eic->height != pre_height) {
            if (sws_ctx) {
                sws_freeContext(sws_ctx);
                sws_ctx = NULL;
            }
            if (!(sws_ctx = sws_getContext(eic->width, eic->height, eic->pix_fmt, eoc->width, eoc->height, eoc->pix_fmt, SWS_BILINEAR, NULL, NULL, NULL))) {
                re.e = UGOIRA_UNABLE_SCALE;
                goto end;
            }
            pre_pixfmt = eic->pix_fmt;
            pre_width = eic->width;
            pre_height = eic->height;
        }
        while (1) {
            if ((re.fferr = av_read_frame(ic, &pkt)) < 0) {
                re.e = UGOIRA_FFMPEG_ERROR;
                goto end;
            }
            if (pkt.data == NULL) {
                av_packet_unref(&pkt);
                re.e = UGOIRA_FFMPEG_ERROR;
                goto end;
            }
            if (pkt.stream_index != is->index) {
                av_packet_unref(&pkt);
                continue;
            }
            if ((re.fferr = avcodec_send_packet(eic, &pkt)) < 0) {
                av_packet_unref(&pkt);
                re.e = UGOIRA_FFMPEG_ERROR;
                goto end;
            }
            av_packet_unref(&pkt);
            re.fferr = avcodec_receive_frame(eic, ifr);
            if (re.fferr >= 0) {
                if ((re.fferr = av_frame_make_writable(ofr)) < 0) {
                    re.e = UGOIRA_FFMPEG_ERROR;
                    goto end;
                }
                if ((re.fferr = sws_scale(sws_ctx, (const uint8_t* const*)ifr->data, ifr->linesize, 0, ifr->height, ofr->data, ofr->linesize)) < 0) {
                    re.e = UGOIRA_FFMPEG_ERROR;
                    goto end;
                }
                re.fferr = 0;
            }
            else if (re.fferr == AVERROR(EAGAIN)) {
                re.fferr = 0;
                continue;
            } else {
                re.e = UGOIRA_FFMPEG_ERROR;
                goto end;
            }
            max_de += av_rescale_q_rnd(frames[i]->delay, tb, os->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
            while (pts < max_de) {
                if (ugoira_encode_video(&re, ofr, oc, eoc, &writed, &pts, os->index, time_base) != UGOIRA_OK) {
                    goto end;
                }
            }
            break;
        }
    }
    if (os) {
        while (1) {
            if (ugoira_encode_video(&re, NULL, oc, eoc, &writed, NULL, os->index, time_base) != UGOIRA_OK) {
                goto end;
            }
            if (!writed) {
                break;
            }
        }
    }
    av_write_trailer(oc);
end:
    if (ifr) av_frame_free(&ifr);
    if (ofr) av_frame_free(&ofr);
    if (sws_ctx) sws_freeContext(sws_ctx);
    if (eic) avcodec_free_context(&eic);
    if (eoc) avcodec_free_context(&eoc);
    if (oc) {
        if (!(oc->oformat->flags & AVFMT_NOFILE)) avio_closep(&oc->pb);
        avformat_free_context(oc);
    }
    if (ic) avformat_close_input(&ic);
    if (mf) {
        free_memfile(mf);
    }
    if (iioc) {
        av_freep(&iioc->buffer);
        avio_context_free(&iioc);
    }
    if (sf) {
        free_zipfile(sf);
    }
    if (opt) {
        av_dict_free(&opt);
    }
    return re;
}
