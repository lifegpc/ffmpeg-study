#include "tg_thumbnail.h"
#include <string.h>
#include "cfileop.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

void init_tg_thumbnail_error(TG_THUMBNAIL_ERROR* err) {
    if (!err) return;
    memset(err, 0, sizeof(TG_THUMBNAIL_ERROR));
}

const char* tg_thumbnail_error_msg(TG_THUMBNAIL_ERROR err) {
    switch (err.e)
    {
    case TG_THUMBNAIL_OK:
        return "OK";
    case TG_THUMBNAIL_FFMPEG_ERROR:
        return "A error occured in ffmpeg code.";
    case TG_THUMBNAIL_NULL_POINTER:
        return "Arguments have null pointers.";
    case TG_THUMBNAIL_REMOVE_OUTPUT_FILE_FAILED:
        return "Can not remove output file.";
    case TG_THUMBNAIL_NO_VIDEO_STREAM:
        return "Can not find video stream in source file.";
    case TG_THUMBNAIL_OOM:
        return "Out of memory.";
    case TG_THUMBNAIL_NO_DECODER:
        return "No available decoder.";
    case TG_THUMBNAIL_NO_ENCODER:
        return "No available encoder.";
    case TG_THUMBNAIL_UNABLE_SCALE:
        return "Unable to scale image.";
    default:
        return "Unknown error.";
    }
}

const char* tg_thumbnail_type_name(TG_THUMBNAIL_TYPE typ) {
    switch (typ)
    {
    case TG_THUMBNAIL_JPEG:
        return "mjpeg";
    case TG_THUMBNAIL_WEBP:
        return "webp";
    default:
        return NULL;
    }
}

int tg_thumbnail_cal_output_size(int iw, int ih, int* ow, int* oh) {
    if (!ow || !oh) return 1;
    if (iw >= 320 && ih >= 320) {
        *ow = 320;
        *oh = 320;
    } else if (iw >= 320) {
        *ow = 320;
        *oh = ih;
    } else if (ih >= 320) {
        *ow = iw;
        *oh = 320;
    } else {
        *ow = iw;
        *oh = ih;
    }
    return 0;
}

int tg_thumbnail_convert_frame(TG_THUMBNAIL_ERROR* err, AVFrame* ifr, AVFrame** ofr, enum AVPixelFormat pxfmt) {
    if (!err || !ifr || !ofr) return 1;
    int re = 0;
    struct SwsContext* sws = NULL;
    AVFrame* fr = NULL, * fr2 = NULL;
    if (!(fr = av_frame_alloc())) {
        err->e = TG_THUMBNAIL_OOM;
        re = 1;
        goto end;
    }
    char simple_way = ifr->width == ifr->height || (ifr->width <= 320 && ifr->height <= 320) ? 1 : 0;
    if (simple_way) {
        if (ifr->width == ifr->height) {
            fr->width = 320;
            fr->height = 320;
        } else {
            fr->width = ifr->width;
            fr->height = ifr->height;
        }
        fr->format = pxfmt;
        fr->sample_aspect_ratio = ifr->sample_aspect_ratio;
    } else {
        fr->width = ifr->width;
        fr->height = ifr->height;
        fr->format = pxfmt;
        fr->sample_aspect_ratio = ifr->sample_aspect_ratio;
    }
    if ((err->fferr = av_frame_get_buffer(fr, 0)) < 0) {
        err->e = TG_THUMBNAIL_FFMPEG_ERROR;
        re = 1;
        goto end;
    }
    if ((err->fferr = av_frame_make_writable(fr)) < 0) {
        err->e = TG_THUMBNAIL_FFMPEG_ERROR;
        re = 1;
        goto end;
    }
    if (!(sws = sws_getContext(ifr->width, ifr->height, (enum AVPixelFormat)ifr->format, fr->width, fr->height, (enum AVPixelFormat)fr->format, SWS_BILINEAR, NULL, NULL, NULL))) {
        err->e = TG_THUMBNAIL_UNABLE_SCALE;
        re = 1;
        goto end;
    }
    if ((err->fferr = sws_scale(sws, (const uint8_t* const*)ifr->data, ifr->linesize, 0, ifr->height, fr->data, fr->linesize)) < 0) {
        err->e = TG_THUMBNAIL_UNABLE_SCALE;
        re = 1;
        goto end;
    }
    if (!simple_way) {
        if (!(fr2 = av_frame_alloc())) {
            err->e = TG_THUMBNAIL_OOM;
            re = 1;
            goto end;
        }
        int width = ifr->width >= ifr->height ? max(ifr->height, 300) : ifr->width;
        int height = ifr->width >= ifr->height ? ifr->height : max(ifr->width, 300);
        int x1 = (ifr->width - width) / 2;
        int y1 = (ifr->height - height) / 2;
        fr2->width = width;
        fr2->height = height;
        fr2->format = pxfmt;
        fr2->sample_aspect_ratio = ifr->sample_aspect_ratio;
        if ((err->fferr = av_frame_get_buffer(fr2, 0)) < 0) {
            err->e = TG_THUMBNAIL_FFMPEG_ERROR;
            re = 1;
            goto end;
        }
        if ((err->fferr = av_frame_make_writable(fr2)) < 0) {
            err->e = TG_THUMBNAIL_FFMPEG_ERROR;
            re = 1;
            goto end;
        }
        int i, j;
        for (i = 0; i < height; i++) {
            memcpy(fr2->data[0] + i * fr2->linesize[0], fr->data[0] + (i + y1) * fr->linesize[0] + x1, width);
        }
        for (j = 1; j < 3; j++) {
            for (i = 0; i < height / 2; i++) {
                memcpy(fr2->data[j] + i * fr2->linesize[j], fr->data[j] + (i + y1 / 2) * fr->linesize[j] + x1 / 2, width / 2);
            }
        }
        if (fr) av_frame_free(&fr);
        if (fr2->width == fr2->height) {
            if (!(fr = av_frame_alloc())) {
                err->e = TG_THUMBNAIL_OOM;
                re = 1;
                goto end;
            }
            fr->width = 320;
            fr->height = 320;
            fr->format = pxfmt;
            fr->sample_aspect_ratio = ifr->sample_aspect_ratio;
            if ((err->fferr = av_frame_get_buffer(fr, 0)) < 0) {
                err->e = TG_THUMBNAIL_FFMPEG_ERROR;
                re = 1;
                goto end;
            }
            if ((err->fferr = av_frame_make_writable(fr)) < 0) {
                err->e = TG_THUMBNAIL_FFMPEG_ERROR;
                re = 1;
                goto end;
            }
            if (sws) sws_freeContext(sws);
            if (!(sws = sws_getContext(fr2->width, fr2->height, (enum AVPixelFormat)fr2->format, fr->width, fr->height, (enum AVPixelFormat)fr->format, SWS_BILINEAR, NULL, NULL, NULL))) {
                err->e = TG_THUMBNAIL_UNABLE_SCALE;
                re = 1;
                goto end;
            }
            if ((err->fferr = sws_scale(sws, (const uint8_t* const*)fr2->data, fr2->linesize, 0, fr2->height, fr->data, fr->linesize)) < 0) {
                err->e = TG_THUMBNAIL_UNABLE_SCALE;
                re = 1;
                goto end;
            }
        } else {
            fr = fr2;
            fr2 = NULL;
        }
    }
end:
    if (re == 1 && fr) av_frame_free(&fr);
    else if (re == 0) *ofr = fr;
    if (fr2) av_frame_free(&fr2);
    if (sws) sws_freeContext(sws);
    return re;
}

int tg_thumbnail_encode_video(TG_THUMBNAIL_ERROR* err, AVFrame* ofr, AVFormatContext* oc, AVCodecContext* occ, char* writed_data) {
    if (!err || !oc || !occ || !writed_data) return 1;
    int re = 0;
    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        err->e = TG_THUMBNAIL_OOM;
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
            err->e = TG_THUMBNAIL_FFMPEG_ERROR;
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
        err->e = TG_THUMBNAIL_FFMPEG_ERROR;
        re = 1;
        goto end;
    }
    if (*writed_data && pkt) {
        pkt->stream_index = 0;
        if ((err->fferr = av_write_frame(oc, pkt)) < 0) {
            err->e = TG_THUMBNAIL_FFMPEG_ERROR;
            re = 1;
            goto end;
        }
    }
end:
    if (pkt) av_packet_free(&pkt);
    return re;
}

TG_THUMBNAIL_RESULT convert_to_tg_thumbnail(const char* src, const char* dest, TG_THUMBNAIL_TYPE format) {
    TG_THUMBNAIL_RESULT re;
    memset(&re, 0, sizeof(TG_THUMBNAIL_RESULT));
    AVFormatContext* ic = NULL, * oc = NULL;
    AVStream* is = NULL, * os = NULL;
    const AVCodec* input_codec = NULL, * output_codec = NULL;
    AVCodecContext* icc = NULL, * occ = NULL;
    char use_copy_method = 0;
    AVPacket pkt;
    AVFrame* ifr = NULL, * ofr = NULL;
    if (!src || !dest) {
        re.err.e = TG_THUMBNAIL_NULL_POINTER;
        goto end;
    }
    if (fileop_exists(dest)) {
        if (!fileop_remove(dest)) {
            re.err.e = TG_THUMBNAIL_REMOVE_OUTPUT_FILE_FAILED;
            goto end;
        }
    }
    if ((re.err.fferr = avformat_open_input(&ic, src, NULL, NULL)) < 0) {
        re.err.e = TG_THUMBNAIL_FFMPEG_ERROR;
        goto end;
    }
    if ((re.err.fferr = avformat_find_stream_info(ic, NULL)) < 0) {
        re.err.e = TG_THUMBNAIL_FFMPEG_ERROR;
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
        re.err.e = TG_THUMBNAIL_NO_VIDEO_STREAM;
        goto end;
    }
    if ((re.err.fferr = avformat_alloc_output_context2(&oc, NULL, tg_thumbnail_type_name(format), dest)) < 0) {
        re.err.e = TG_THUMBNAIL_FFMPEG_ERROR;
        goto end;
    }
    if (is->codecpar->width <= 320 && is->codecpar->height <= 320) {
        if ((is->codecpar->codec_id == AV_CODEC_ID_MJPEG && format == TG_THUMBNAIL_JPEG && is->codecpar->format == AV_PIX_FMT_YUVJ420P) || (is->codecpar->codec_id == AV_CODEC_ID_WEBP && format == TG_THUMBNAIL_WEBP && is->codecpar->format == AV_PIX_FMT_YUV420P)) {
            if (!(os = avformat_new_stream(oc, NULL))) {
                re.err.e = TG_THUMBNAIL_OOM;
                goto end;
            }
            if ((re.err.fferr = avcodec_parameters_copy(os->codecpar, is->codecpar)) < 0) {
                re.err.e = TG_THUMBNAIL_FFMPEG_ERROR;
                goto end;
            }
            os->codecpar->codec_tag = 0;
            use_copy_method = 1;
        }
    }
    if (!os) {
        if (!(input_codec = avcodec_find_decoder(is->codecpar->codec_id))) {
            re.err.e = TG_THUMBNAIL_NO_DECODER;
            goto end;
        }
        if (!(icc = avcodec_alloc_context3(input_codec))) {
            re.err.e = TG_THUMBNAIL_OOM;
            goto end;
        }
        if ((re.err.fferr = avcodec_parameters_to_context(icc, is->codecpar)) < 0) {
            re.err.e = TG_THUMBNAIL_FFMPEG_ERROR;
            goto end;
        }
        if ((re.err.fferr = avcodec_open2(icc, input_codec, NULL)) < 0) {
            re.err.e = TG_THUMBNAIL_FFMPEG_ERROR;
            goto end;
        }
        output_codec = format == TG_THUMBNAIL_JPEG ? avcodec_find_encoder(AV_CODEC_ID_MJPEG) : avcodec_find_encoder(AV_CODEC_ID_WEBP);
        if (!(occ = avcodec_alloc_context3(output_codec))) {
            re.err.e = TG_THUMBNAIL_OOM;
            goto end;
        }
        if (tg_thumbnail_cal_output_size(icc->width, icc->height, &occ->width, &occ->height)) {
            re.err.e = TG_THUMBNAIL_NULL_POINTER;
            goto end;
        }
        occ->pix_fmt = format == TG_THUMBNAIL_WEBP ? AV_PIX_FMT_YUV420P : AV_PIX_FMT_YUVJ420P;
        occ->sample_aspect_ratio = icc->sample_aspect_ratio;
        occ->time_base = AV_TIME_BASE_Q;
        if (format == TG_THUMBNAIL_JPEG) occ->color_range = AVCOL_RANGE_JPEG;
        if ((re.err.fferr = avcodec_open2(occ, output_codec, NULL)) < 0) {
            re.err.e = TG_THUMBNAIL_FFMPEG_ERROR;
            goto end;
        }
        if (!(os = avformat_new_stream(oc, NULL))) {
            re.err.e = TG_THUMBNAIL_OOM;
            goto end;
        }
        if ((re.err.fferr = avcodec_parameters_from_context(os->codecpar, occ)) < 0) {
            re.err.e = TG_THUMBNAIL_FFMPEG_ERROR;
            goto end;
        }
    }
    av_dump_format(oc, 0, dest, 1);
    if (!(oc->oformat->flags & AVFMT_NOFILE)) {
        if ((re.err.fferr = avio_open(&oc->pb, dest, AVIO_FLAG_WRITE)) < 0) {
            re.err.e = TG_THUMBNAIL_FFMPEG_ERROR;
            goto end;
        }
    }
    if ((re.err.fferr = avformat_write_header(oc, NULL)) < 0) {
        re.err.e = TG_THUMBNAIL_FFMPEG_ERROR;
        goto end;
    }
    while (1) {
        if ((re.err.fferr = av_read_frame(ic, &pkt)) < 0) {
            re.err.e = TG_THUMBNAIL_FFMPEG_ERROR;
            goto end;
        }
        if (pkt.data == NULL) {
            av_packet_unref(&pkt);
            re.err.e = TG_THUMBNAIL_FFMPEG_ERROR;
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
                re.err.e = TG_THUMBNAIL_FFMPEG_ERROR;
                goto end;
            }
        } else {
            if (!(ifr = av_frame_alloc())) {
                av_packet_unref(&pkt);
                re.err.e = TG_THUMBNAIL_OOM;
                goto end;
            }
            if ((re.err.fferr = avcodec_send_packet(icc, &pkt)) < 0) {
                av_packet_unref(&pkt);
                re.err.e = TG_THUMBNAIL_FFMPEG_ERROR;
                goto end;
            }
            if ((re.err.fferr = avcodec_receive_frame(icc, ifr)) < 0) {
                if (re.err.fferr == AVERROR(EAGAIN)) {
                    av_packet_unref(&pkt);
                    re.err.fferr = 0;
                    continue;
                }
                av_packet_unref(&pkt);
                re.err.e = TG_THUMBNAIL_FFMPEG_ERROR;
                goto end;
            }
            if (tg_thumbnail_convert_frame(&re.err, ifr, &ofr, occ->pix_fmt)) {
                av_packet_unref(&pkt);
                re.err.e = TG_THUMBNAIL_FFMPEG_ERROR;
                goto end;
            }
            char writed = 0;
            if (tg_thumbnail_encode_video(&re.err, ofr, oc, occ, &writed)) {
                av_packet_unref(&pkt);
                re.err.e = TG_THUMBNAIL_FFMPEG_ERROR;
                goto end;
            }
            while (1) {
                if (tg_thumbnail_encode_video(&re.err, NULL, oc, occ, &writed)) {
                    av_packet_unref(&pkt);
                    re.err.e = TG_THUMBNAIL_FFMPEG_ERROR;
                    goto end;
                }
                if (!writed) break;
            }
        }
        av_packet_unref(&pkt);
        break;
    }
    av_write_trailer(oc);
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
