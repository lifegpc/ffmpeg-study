#if HAVE_FFCONCAT_CONFIG_H
#include "ffconcat_config.h"
#endif

#include "ffconcat.h"
extern "C" {
    #include "libavutil/log.h"
    #include "libavutil/timestamp.h"
    #include "libavformat/avformat.h"

    static inline enum AVRounding to_avround(int i) {
        return (enum AVRounding)i;
    }
}

#if HAVE_PRINTF_S
#define printf printf_s
#endif

void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag) {
    char buf[AV_TS_MAX_STRING_SIZE], buf2[AV_TS_MAX_STRING_SIZE], buf3[AV_TS_MAX_STRING_SIZE],
    buf4[AV_TS_MAX_STRING_SIZE], buf5[AV_TS_MAX_STRING_SIZE], buf6[AV_TS_MAX_STRING_SIZE];
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;
    printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           tag,
           av_ts_make_string(buf, pkt->pts), av_ts_make_time_string(buf2, pkt->pts, time_base),
           av_ts_make_string(buf3, pkt->dts), av_ts_make_time_string(buf4, pkt->dts, time_base),
           av_ts_make_string(buf5, pkt->duration), av_ts_make_time_string(buf6, pkt->duration, time_base),
           pkt->stream_index);
}

int ffconcat(std::string out, std::list<std::string> inp, ffconcath config) {
    if (inp.size() == 0) {
        printf("Error: %s\n", "No input file specified.");
        return 1;
    }
    if (config.trace) {
        av_log_set_level(AV_LOG_TRACE);
    } else if (config.debug) {
        av_log_set_level(AV_LOG_DEBUG);
    } else if (config.verbose) {
        av_log_set_level(AV_LOG_VERBOSE);
    }
    AVFormatContext *oc = nullptr, *ic = nullptr;
    int ret, rev = 0;
    AVPacket pkt;
    avformat_alloc_output_context2(&oc, nullptr, nullptr, out.c_str());
    if (oc == nullptr) {
        printf("Warning: %s\n", "Can not detect format from output file extension. Assume to use MPEG.");
        avformat_alloc_output_context2(&oc, nullptr, "MPEG", out.c_str());
        if (oc == nullptr) {
            printf("Error: %s\n", "Can not create output context.");
            return 1;
        }
    }
    if (config.verbose) {
        printf("%s\n", "Open output context successfully.");
        printf("Use \"%s\" format to mux files.\n", oc->oformat->name);
    }
    auto i = inp.begin();
    if ((ret = avformat_open_input(&ic, (*i).c_str(), nullptr, nullptr)) != 0) {
        rev = 2;
        goto end;
    }
    if ((ret = avformat_find_stream_info(ic, nullptr)) < 0) {
        rev = 3;
        goto end;
    }
    av_dump_format(ic, 0, (*i).c_str(), 0);
    int map_size = ic->nb_streams, map_index = 0;
    int* map = (int*) calloc(map_size, sizeof(int));
    if (!map) {
        printf("%s\n", "Can not allocate memory.");
        rev = 4;
        goto end;
    }
    for (int i = 0; i < map_size; i++) {
        AVStream *os, *is = ic->streams[i];
        auto cpr = is->codecpar;
        auto st = cpr->codec_type;
        if (st != AVMEDIA_TYPE_VIDEO || st != AVMEDIA_TYPE_AUDIO || st != AVMEDIA_TYPE_SUBTITLE) {
            map[i] = -1;
        }
        map[i] = map_index++;

        os = avformat_new_stream(oc, nullptr);
        if (!os) {
            printf("%s\n", "Can not allocate memory for output stream.");
            rev = 4;
            goto end;
        }

        if ((ret = avcodec_parameters_copy(os->codecpar, cpr)) < 0) {
            printf("%s\n", "Can not copy stream parameters.");
            rev = 5;
            goto end;
        }
        if (oc->oformat->name && !strcmp(oc->oformat->name, "ipod")) {
            if (is->codecpar->codec_id == AV_CODEC_ID_MJPEG) {
                os->disposition = os->disposition | AV_DISPOSITION_ATTACHED_PIC;
            }
        }
        os->codecpar->codec_tag = 0;
    }
    av_dump_format(oc, 0, out.c_str(), 1);
    if (!(oc->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, out.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            printf("Could not open output file '%s'\n", out.c_str());
            goto end;
        }
    }
    if ((ret = avformat_write_header(oc, nullptr)) < 0) {
        printf("Can not write file header.");
        rev = 6;
        goto end;
    }
    int64_t duration = 0;
    do {
        if (i != inp.begin()) {
            if ((ret = avformat_open_input(&ic, (*i).c_str(), nullptr, nullptr)) != 0) {
                rev = 2;
                goto end;
            }
            if ((ret = avformat_find_stream_info(ic, nullptr)) < 0) {
                rev = 3;
                goto end;
            }
            if (config.verbose) {
                av_dump_format(ic, 0, (*i).c_str(), 0);
            }
        }
        while (true) {
            AVStream *is, *os;
            if ((ret = av_read_frame(ic, &pkt)) < 0) {
                if (ret == AVERROR_EOF) break;
                rev = 7;
                goto end;
            }
            is = ic->streams[pkt.stream_index];
            if (pkt.stream_index >= map_size || map[pkt.stream_index] < 0) {
                av_packet_unref(&pkt);
                continue;
            }
            pkt.stream_index = map[pkt.stream_index];
            os = oc->streams[pkt.stream_index];
            if (config.trace) {
                log_packet(ic, &pkt, "in");
            }
            AVRational base = {1, AV_TIME_BASE};
            int64_t delta = av_rescale_q(duration, base, os->time_base);
            pkt.pts = av_rescale_q_rnd(pkt.pts, is->time_base, os->time_base, to_avround(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX)) + delta;
            pkt.dts = av_rescale_q_rnd(pkt.dts, is->time_base, os->time_base, to_avround(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX)) + delta;
            pkt.duration = av_rescale_q(pkt.duration, is->time_base, os->time_base);
            pkt.pos = -1;
            if (config.trace) {
                log_packet(oc, &pkt, "out");
            }
            if ((ret = av_interleaved_write_frame(oc, &pkt)) < 0) {
                printf("Error muxing packet\n");
                break;
            }
            av_packet_unref(&pkt);
        }
        duration += ic->duration;
        avformat_close_input(&ic);
        i++;
    } while (i != inp.end());
    av_write_trailer(oc);
end:
    if (oc) {
        if (!(oc->oformat->flags & AVFMT_NOFILE)) avio_closep(&oc->pb);
        avformat_free_context(oc);
    }
    if (ic) avformat_close_input(&ic);
    if (map) free(map);
    if (ret < 0 && ret != AVERROR_EOF) {
        char err[AV_ERROR_MAX_STRING_SIZE];
        printf("Error occurred: %s\n", av_make_error_string(err, AV_ERROR_MAX_STRING_SIZE, ret));
    }
    return rev;
}
