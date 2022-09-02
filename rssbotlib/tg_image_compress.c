#include "rssbotlib_config.h"
#include "tg_image_compress.h"
#include <string.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "cfileop.h"
#include "utils.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

void init_tg_image_error(TG_IMAGE_ERROR* err) {
    if (!err) return;
    memset(err, 0, sizeof(TG_IMAGE_ERROR));
}

const char* tg_image_error_msg(TG_IMAGE_ERROR err) {
    switch (err.e)
    {
    case TG_IMAGE_OK:
        return "OK";
    case TG_IMAGE_FFMPEG_ERROR:
        return "A error occured in ffmpeg code.";
    case TG_IMAGE_NULL_POINTER:
        return "Arguments have null pointers.";
    case TG_IMAGE_REMOVE_OUTPUT_FILE_FAILED:
        return "Can not remove output file.";
    case TG_IMAGE_NO_VIDEO_STREAM:
        return "Can not find video stream in source file.";
    case TG_IMAGE_OOM:
        return "Out of memory.";
    case TG_IMAGE_NO_DECODER:
        return "No available decoder.";
    case TG_IMAGE_NO_ENCODER:
        return "No available encoder.";
    case TG_IMAGE_UNABLE_SCALE:
        return "Unable to scale image.";
    default:
        return "Unknown error.";
    }
}

const char* tg_image_type_name(TG_IMAGE_TYPE type) {
    switch (type)
    {
    case TG_IMAGE_JPEG:
        return "mjpeg";
    case TG_IMAGE_PNG:
        return "png";
    default:
        return NULL;
    }
}

int tg_image_cal_output_size(int iw, int ih, int ml, int *ow, int *oh) {
    if (!ow || !oh) return 1;
    AVRational base = { 1, 1 };
    if (iw >= ih) {
        *ow = ml;
        AVRational tmp = { ih, iw };
        *oh = (int)av_rescale_q_rnd(ml, tmp, base, AV_ROUND_NEAR_INF);
    } else {
        *oh = ml;
        AVRational tmp = { iw, ih };
        *ow = (int)av_rescale_q_rnd(ml, tmp, base, AV_ROUND_NEAR_INF);
    }
    return 0;
}

int tg_image_convert_frame(TG_IMAGE_ERROR* err, AVFrame* ifr, AVFrame** ofr, AVCodecContext* occ) {
    if (!err || !ifr || !ofr || !occ) return 1;
    int re = 0;
    struct SwsContext* sws = NULL;
    AVFrame* fr = NULL;
    if (!(fr = av_frame_alloc())) {
        err->e = TG_IMAGE_OOM;
        re = 1;
        goto end;
    }
    fr->width = occ->width;
    fr->height = occ->height;
    fr->format = occ->pix_fmt;
    fr->sample_aspect_ratio = occ->sample_aspect_ratio;
    if ((err->fferr = av_frame_get_buffer(fr, 0)) < 0) {
        err->e = TG_IMAGE_FFMPEG_ERROR;
        re = 1;
        goto end;
    }
    if ((err->fferr = av_frame_make_writable(fr)) < 0) {
        err->e = TG_IMAGE_FFMPEG_ERROR;
        re = 1;
        goto end;
    }
    if (!(sws = sws_getContext(ifr->width, ifr->height, (enum AVPixelFormat)ifr->format, fr->width, fr->height, (enum AVPixelFormat)fr->format, SWS_BICUBIC, NULL, NULL, NULL))) {
        err->e = TG_IMAGE_UNABLE_SCALE;
        re = 1;
        goto end;
    }
    if ((err->fferr = sws_scale(sws, (const uint8_t* const*)ifr->data, ifr->linesize, 0, ifr->height, fr->data, fr->linesize)) < 0) {
        err->e = TG_IMAGE_UNABLE_SCALE;
        re = 1;
        goto end;
    }
end:
    if (re == 1 && fr) av_frame_free(&fr);
    else if (re == 0) *ofr = fr;
    if (sws) sws_freeContext(sws);
    return re;
}

int tg_image_encode_video(TG_IMAGE_ERROR* err, AVFrame* ofr, AVFormatContext* oc, AVCodecContext* occ, char* writed_data) {
    if (!err || !oc || !occ || !writed_data) return 1;
    int re = 0;
    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        err->e = TG_IMAGE_OOM;
        re = 1;
        goto end;
    }
    *writed_data = 0;
    if (ofr) {
        ofr->pts = 0;
        ofr->pkt_dts = 0;
    }
    if ((err->fferr = avcodec_send_frame(occ, ofr)) < 0) {
        if (err->fferr == AVERROR_EOF) {
            err->fferr = 0;
        } else {
            err->e = TG_IMAGE_FFMPEG_ERROR;
            re = 1;
            goto end;
        }
    }
    err->fferr = avcodec_receive_packet(occ, pkt);
    if (err->fferr >= 0) {
        *writed_data = 1;
    } else if (err->fferr == AVERROR_EOF || err->fferr == AVERROR(EAGAIN)) {
        err->fferr = 0;
        goto end;
    } else {
        err->e = TG_IMAGE_FFMPEG_ERROR;
        re = 1;
        goto end;
    }
    if (*writed_data && pkt) {
        pkt->stream_index = 0;
        if ((err->fferr = av_write_frame(oc, pkt)) < 0) {
            err->e = TG_IMAGE_FFMPEG_ERROR;
            re = 1;
            goto end;
        }
    }
end:
    if (pkt) av_packet_free(&pkt);
    return re;
}

TG_IMAGE_RESULT tg_image_compress(const char* src, const char* dest, TG_IMAGE_TYPE type, int max_len, AVDictionary* opts) {
    TG_IMAGE_RESULT re;
    memset(&re, 0, sizeof(TG_IMAGE_RESULT));
    AVFormatContext* ic = NULL, * oc = NULL;
    AVStream* is = NULL, * os = NULL;
    const AVCodec* input_codec = NULL, * output_codec = NULL;
    AVCodecContext* icc = NULL, * occ = NULL;
    AVPacket pkt;
    char use_copy_method = 0, force_yuv420p = 1;
    AVFrame* ifr = NULL, * ofr = NULL;
    int omax_len = 0;
    if (!src || !dest) {
        re.err.e = TG_IMAGE_NULL_POINTER;
        goto end;
    }
    if (opts) {
        AVDictionaryEntry* tmp = av_dict_get(opts, "force_yuv420p", NULL, 0);
        if (tmp) {
#if HAVE_SSCANF_S
            if (sscanf_s(tmp->value, "%c", &force_yuv420p, 1) != 1) {
#else
            if (sscanf(tmp->value, "%c", &force_yuv420p) != 1) {
#endif
                force_yuv420p = 1;
            }
        }
    }
    if (fileop_exists(dest)) {
        if (!fileop_remove(dest)) {
            re.err.e = TG_IMAGE_REMOVE_OUTPUT_FILE_FAILED;
            goto end;
        }
    }
    if ((re.err.fferr = avformat_open_input(&ic, src, NULL, NULL)) < 0) {
        re.err.e = TG_IMAGE_FFMPEG_ERROR;
        goto end;
    }
    if ((re.err.fferr = avformat_find_stream_info(ic, NULL)) < 0) {
        re.err.e = TG_IMAGE_FFMPEG_ERROR;
        goto end;
    }
    av_dump_format(ic, 0, src, 0);
    for (unsigned int i = 0; i < ic->nb_streams; i++) {
        AVStream* s = ic->streams[i];
        if (s->codecpar && s->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            is = s;
            break;
        }
    }
    if (!is) {
        re.err.e = TG_IMAGE_NO_VIDEO_STREAM;
        goto end;
    }
    if ((re.err.fferr = avformat_alloc_output_context2(&oc, NULL, tg_image_type_name(type), dest)) < 0) {
        re.err.e = TG_IMAGE_FFMPEG_ERROR;
        goto end;
    }
    omax_len = max(is->codecpar->width, is->codecpar->height);
    if (omax_len <= max_len) {
        if ((is->codecpar->codec_id == AV_CODEC_ID_MJPEG && type == TG_IMAGE_JPEG && (is->codecpar->format == AV_PIX_FMT_YUVJ420P || !force_yuv420p)) || (is->codecpar->codec_id == AV_CODEC_ID_PNG && type == TG_IMAGE_PNG && (is->codecpar->format == AV_PIX_FMT_YUV420P || !force_yuv420p))) {
            if (!(os = avformat_new_stream(oc, NULL))) {
                re.err.e = TG_IMAGE_OOM;
                goto end;
            }
            if ((re.err.fferr = avcodec_parameters_copy(os->codecpar, is->codecpar)) < 0) {
                re.err.e = TG_IMAGE_FFMPEG_ERROR;
                goto end;
            }
            os->codecpar->codec_tag = 0;
            use_copy_method = 1;
        }
    }
    if (!os) {
        if (!(input_codec = avcodec_find_decoder(is->codecpar->codec_id))) {
            re.err.e = TG_IMAGE_NO_DECODER;
            goto end;
        }
        if (!(icc = avcodec_alloc_context3(input_codec))) {
            re.err.e = TG_IMAGE_OOM;
            goto end;
        }
        if ((re.err.fferr = avcodec_parameters_to_context(icc, is->codecpar)) < 0) {
            re.err.e = TG_IMAGE_FFMPEG_ERROR;
            goto end;
        }
        if ((re.err.fferr = avcodec_open2(icc, input_codec, NULL)) < 0) {
            re.err.e = TG_IMAGE_FFMPEG_ERROR;
            goto end;
        }
        output_codec = avcodec_find_encoder(type == TG_IMAGE_JPEG ? AV_CODEC_ID_MJPEG : AV_CODEC_ID_PNG);
        if (!output_codec) {
            re.err.e = TG_IMAGE_NO_ENCODER;
            goto end;
        }
        if (!(occ = avcodec_alloc_context3(output_codec))) {
            re.err.e = TG_IMAGE_OOM;
            goto end;
        }
        if (tg_image_cal_output_size(icc->width, icc->height, max_len, &occ->width, &occ->height)) {
            re.err.e = TG_IMAGE_NULL_POINTER;
            goto end;
        }
        occ->pix_fmt = type == TG_IMAGE_JPEG ? AV_PIX_FMT_YUVJ420P : AV_PIX_FMT_YUV420P;
        if (!force_yuv420p && is_supported_pixfmt(icc->pix_fmt, output_codec->pix_fmts)) {
            occ->pix_fmt = icc->pix_fmt;
        }
        occ->sample_aspect_ratio = icc->sample_aspect_ratio;
        occ->time_base = AV_TIME_BASE_Q;
        if (type == TG_IMAGE_JPEG) {
            occ->color_range = AVCOL_RANGE_JPEG;
        }
        if ((re.err.fferr = avcodec_open2(occ, output_codec, NULL)) < 0) {
            re.err.e = TG_IMAGE_FFMPEG_ERROR;
            goto end;
        }
        if (!(os = avformat_new_stream(oc, NULL))) {
            re.err.e = TG_IMAGE_OOM;
            goto end;
        }
        if ((re.err.fferr = avcodec_parameters_from_context(os->codecpar, occ)) < 0) {
            re.err.e = TG_IMAGE_FFMPEG_ERROR;
            goto end;
        }
    }
    av_dump_format(oc, 0, dest, 1);
    if (!(oc->oformat->flags & AVFMT_NOFILE)) {
        if ((re.err.fferr = avio_open(&oc->pb, dest, AVIO_FLAG_WRITE)) < 0) {
            re.err.e = TG_IMAGE_FFMPEG_ERROR;
            goto end;
        }
    }
    if ((re.err.fferr = avformat_write_header(oc, NULL)) < 0) {
        re.err.e = TG_IMAGE_FFMPEG_ERROR;
        goto end;
    }
    while (1) {
        if ((re.err.fferr = av_read_frame(ic, &pkt)) < 0) {
            re.err.e = TG_IMAGE_FFMPEG_ERROR;
            goto end;
        }
        if (pkt.data == NULL) {
            av_packet_unref(&pkt);
            re.err.e = TG_IMAGE_FFMPEG_ERROR;
            goto end;
        }
        if (pkt.stream_index != is->index) {
            av_packet_unref(&pkt);
            continue;
        }
        if (use_copy_method) {
            pkt.pts = 0;
            pkt.dts = 0;
            pkt.duration = av_rescale_q(pkt.duration, is->time_base, os->time_base);
            pkt.pos = -1;
            pkt.stream_index = os->index;
            if ((re.err.fferr = av_interleaved_write_frame(oc, &pkt)) < 0) {
                av_packet_unref(&pkt);
                re.err.e = TG_IMAGE_FFMPEG_ERROR;
                goto end;
            }
        } else {
            if (!(ifr = av_frame_alloc())) {
                av_packet_unref(&pkt);
                re.err.e = TG_IMAGE_OOM;
                goto end;
            }
            if ((re.err.fferr = avcodec_send_packet(icc, &pkt)) < 0) {
                av_packet_unref(&pkt);
                re.err.fferr = TG_IMAGE_FFMPEG_ERROR;
                goto end;
            }
            if ((re.err.fferr = avcodec_receive_frame(icc, ifr)) < 0) {
                if (re.err.fferr == AVERROR(EAGAIN)) {
                    av_packet_unref(&pkt);
                    re.err.fferr = 0;
                    continue;
                }
                av_packet_unref(&pkt);
                re.err.fferr = TG_IMAGE_FFMPEG_ERROR;
                goto end;
            }
            if (tg_image_convert_frame(&re.err, ifr, &ofr, occ)) {
                av_packet_unref(&pkt);
                re.err.fferr = TG_IMAGE_FFMPEG_ERROR;
                goto end;
            }
            char writed = 0;
            if (tg_image_encode_video(&re.err, ofr, oc, occ, &writed)) {
                av_packet_unref(&pkt);
                re.err.fferr = TG_IMAGE_FFMPEG_ERROR;
                goto end;
            }
            while (1) {
                if (tg_image_encode_video(&re.err, NULL, oc, occ, &writed)) {
                    av_packet_unref(&pkt);
                    re.err.fferr = TG_IMAGE_FFMPEG_ERROR;
                    goto end;
                }
                if (!writed) {
                    break;
                }
            }
        }
        av_packet_unref(&pkt);
        break;
    }
    if ((re.err.fferr = av_write_trailer(oc)) < 0) {
        re.err.e = TG_IMAGE_FFMPEG_ERROR;
        goto end;
    }
    re.width = os->codecpar->width;
    re.height = os->codecpar->height;
end:
    if (ifr) av_frame_free(&ifr);
    if (ofr) av_frame_free(&ofr);
    if (icc) avcodec_free_context(&icc);
    if (occ) avcodec_free_context(&occ);
    if (oc) {
        if (!(oc->oformat->flags & AVFMT_NOFILE)) avio_closep(&oc->pb);
        avformat_free_context(oc);
    }
    if (ic) avformat_close_input(&ic);
    return re;
}
