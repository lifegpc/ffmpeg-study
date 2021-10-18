#if HAVE_ENM4A_CONFIG_H
#include "enm4a_config.h"
#endif

#include "enm4a.h"
#include "enm4a_http_header.h"

#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <time.h>

#include "cstr_util.h"
#include "cfileop.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/log.h"
#include "libavutil/dict.h"
#include "libavutil/timestamp.h"

#ifdef _WIN32
#include <Windows.h>
#define ft2ts(t) (((size_t)t.dwHighDateTime << 32) | (size_t)t.dwLowDateTime)
#endif

#if HAVE_PRINTF_S
#define printf printf_s
#endif

#if HAVE_CLOCK_GETTIME
#define ts2ts(t) (t.tv_sec * 1000000000ll + t.tv_nsec)
#endif

void log_packet(const AVFormatContext* fmt_ctx, const AVPacket* pkt, const char* tag) {
    AVRational* time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;
    printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
        tag, av_ts2str(pkt->pts), av_ts2timestr(pkt->dts, time_base),
        av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
        av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
        pkt->stream_index);
}

#define LOG_PROGRESS_BUFSIZE 256

void log_progress(const AVFormatContext* ctx, int64_t pts, AVRational base) {
    int64_t total_size;
    total_size = avio_size(ctx->pb);
    if (total_size <= 0) {
        total_size = avio_tell(ctx->pb);
    }
    char buf[LOG_PROGRESS_BUFSIZE];
    char* nbuf = buf;
    size_t len = LOG_PROGRESS_BUFSIZE;
    int c = 0;
    if (total_size <= 0) {
        c = snprintf(nbuf, len, "size=N/A");
    } else {
        c = snprintf(nbuf, len, "size=%6.0fKiB", total_size / 1024.0);
    }
    len -= c;
    nbuf += c;
    if (pts < 0) {
        c = snprintf(nbuf, len, "\ttime=N/A");
    } else {
        int bq = base.den / base.num;
        int secs = FFABS(pts) / bq;
        int us = FFABS(pts) % bq;
        int mins = secs / 60;
        secs %= 60;
        int hours = mins / 60;
        mins %= 60;
        const char* hours_sign = (pts < 0) ? "-" : "";
        c = snprintf(nbuf, len, "\ttime=%s%02d:%02d:%02d.%02d", hours_sign, hours, mins, secs, (us * 100) / bq);
    }
    len -= c;
    nbuf += c;
    printf("\r%s", buf);
    fflush(stdout);
}

void set_ctx_metadata(AVFormatContext *ctx, const AVFormatContext *in, const char* key, const char* argu) {
    if (!argu || !strlen(argu)) {
        if (in->metadata) {
            AVDictionaryEntry* en = av_dict_get(in->metadata, key, NULL, 0);
            if (en) {
                av_dict_set(&ctx->metadata, key, en->value, 0);
            }
        }
    } else {
        av_dict_set(&ctx->metadata, key, argu, 0);
    }
}

ENM4A_ERROR encode_m4a(const char* input, ENM4A_ARGS args) {
    if (!input) return ENM4A_NULL_POINTER;
    switch (args.level) {
    case ENM4A_LOG_VERBOSE:
        av_log_set_level(AV_LOG_VERBOSE);
        break;
    case ENM4A_LOG_DEBUG:
        av_log_set_level(AV_LOG_DEBUG);
        break;
    case ENM4A_LOG_TRACE:
        av_log_set_level(AV_LOG_TRACE);
        break;
    }
    AVFormatContext* ic = NULL, * oc = NULL, * imgc = NULL;
    int ret = 0;
    ENM4A_ERROR rev = ENM4A_OK;
    char* title = NULL, * out = NULL, * headers = NULL;
    char has_img = 0, img_extra_file = 0, has_audio = 0;
    unsigned int img_stream_index = 0, audio_stream_index = 0, img_dest_index = 0, audio_dest_index = 0, map_index = 0;
    AVPacket pkt;
    int64_t audio_dts;
    AVDictionary* demux_option = NULL;
#ifdef _WIN32
    FILETIME pgtime = { 0, 0 }, tnow = { 0, 0 };
#elif defined(HAVE_CLOCK_GETTIME)
    struct timespec pgtime = { LLONG_MIN, 0 }, tnow = { 0, 0 };
#else
    time_t pgtime = LLONG_MIN, tnow = 0;
#endif
    if (args.http_headers && args.http_header_size) {
        ENM4A_ERROR err;
        headers = enm4a_generate_http_header(args.http_headers, args.http_header_size, &err);
        if (!headers) {
            rev = err;
            goto end;
        }
        if ((ret = av_dict_set(&demux_option, "headers", headers, 0)) < 0) {
            rev = ENM4A_FFMPEG_ERR;
            goto end;
        }
    }
    if ((ret = avformat_open_input(&ic, input, NULL, &demux_option)) != 0) {
        rev = ENM4A_FFMPEG_ERR;
        goto end;
    }
    if ((ret = avformat_find_stream_info(ic, NULL)) < 0) {
        rev = ENM4A_FFMPEG_ERR;
        goto end;
    }
    av_dump_format(ic, 0, input, 0);
    if (args.cover && strlen(args.cover)) {
        if ((ret = avformat_open_input(&imgc, args.cover, NULL, NULL)) < 0) {
            rev = ENM4A_FFMPEG_ERR;
            goto end;
        }
        if ((ret = avformat_find_stream_info(imgc, NULL)) < 0) {
            rev = ENM4A_FFMPEG_ERR;
            goto end;
        }
        av_dump_format(imgc, 1, args.cover, 0);
    }
    if (!args.title || !strlen(args.title)) {
        if (ic->metadata) {
            AVDictionaryEntry* en = av_dict_get(ic->metadata, "title", NULL, 0);
            if (en) {
                int re = cstr_util_copy_str(&title, en->value);
                if (re) {
                    rev = re == 2 ? ENM4A_NO_MEMORY : ENM4A_NULL_POINTER;
                    goto end;
                }
                if (args.level >= ENM4A_LOG_VERBOSE) {
                    printf("Get title from file metadata : %s\n", title);
                }
            }
        }
    } else {
        int re = cstr_util_copy_str(&title, args.title);
        if (re) {
            rev = re == 2 ? ENM4A_NO_MEMORY : ENM4A_NULL_POINTER;
            goto end;
        }
        if (args.level >= ENM4A_LOG_VERBOSE) {
            printf("Get title from input argument: %s\n", title);
        }
    }
    if (!args.output || !strlen(args.output)) {
        if (title) {
            int is_url = 0;
            if (!fileop_is_url(input, &is_url)) {
                rev = ENM4A_NULL_POINTER;
                goto end;
            }
            char* dir = !is_url ? fileop_dirname(input) : NULL;
            if (!is_url && !dir) {
                rev = ENM4A_NO_MEMORY;
                goto end;
            }
            size_t dle = dir ? strlen(dir) : 0;
            size_t le = strlen(title);
            size_t nle = dle == 0 ? le + 4 : dle + 1 + le + 4;
            out = malloc(nle + 1);
            if (!out) {
                if (dir) free(dir);
                rev = ENM4A_NO_MEMORY;
                goto end;
            }
            if (dle == 0) {
                memcpy(out, title, le);
                memcpy(out + le, ".m4a", 4);
            } else {
                memcpy(out, dir, dle);
#ifdef _WIN32
                out[dle] = '\\';
#else
                out[dle] = '/';
#endif
                memcpy(out + dle + 1, title, le);
                memcpy(out + nle - 4, ".m4a", 4);
            }
            out[nle] = 0;
            if (args.level >= ENM4A_LOG_VERBOSE) {
                printf("Get output filename from title: %s\n", out);
            }
            if (dir) free(dir);
        } else {
            char* bn = NULL;
            int is_url = 0;
            if (!fileop_is_url(input, &is_url)) {
                rev = ENM4A_NULL_POINTER;
                goto end;
            }
            char* ext = NULL;
            size_t le = 0;
            if (is_url) {
                bn = fileop_basename(input);
                if (!bn) {
                    rev = ENM4A_NO_MEMORY;
                    goto end;
                }
                ext = strrchr(bn, '.');
                le = (ext == NULL || !strncmp(ext, ".m4a", 4)) ? strlen(bn) : ext - bn;
            } else {
                ext = strrchr(input, '.');
                le = (ext == NULL || !strncmp(ext, ".m4a", 4)) ? strlen(input) : ext - input;
            }
            out = malloc(le + 5);
            if (!out) {
                if (bn) free(bn);
                rev = ENM4A_NO_MEMORY;
                goto end;
            }
            if (is_url) {
                memcpy(out, bn, le);
            } else {
                memcpy(out, input, le);
            }
            memcpy(out + le, ".m4a", 4);
            out[le + 4] = 0;
            if (args.level >= ENM4A_LOG_VERBOSE) {
                printf("Use default output filename: %s\n", out);
            }
            if (bn) free(bn);
        }
    } else {
        int re = cstr_util_copy_str(&out, args.output);
        if (re) {
            rev = re == 2 ? ENM4A_NO_MEMORY : ENM4A_NULL_POINTER;
            goto end;
        }
        if (args.level >= ENM4A_LOG_VERBOSE) {
            printf("Get output filename from input argument: %s\n", out);
        }
    }
    if (fileop_exists(out)) {
        int overwrite = 0;
        if (args.overwrite != ENM4A_OVERWRITE_ASK) {
            if (args.overwrite == ENM4A_OVERWRITE_YES) overwrite = 1;
        } else {
            printf("Output file already exists, do you want to overwrite it? (y/n)");
            int c = getchar();
            while (c != 'y' && c != 'n') {
                c = getchar();
            }
            if (c == 'y') overwrite = 1;
        }
        if (!overwrite) {
            rev = ENM4A_FILE_EXISTS;
            goto end;
        } else {
            if (!fileop_remove(out)) {
                rev = ENM4A_ERR_REMOVE_FILE;
                goto end;
            }
        }
    }
    if ((ret = avformat_alloc_output_context2(&oc, NULL, "ipod", out)) < 0) {
        rev = ENM4A_FFMPEG_ERR;
        goto end;
    }
    if (imgc) {
        for (unsigned int i = 0; i < imgc->nb_streams; i++) {
            AVStream* is = imgc->streams[i], * os = NULL;
            if (is->codecpar->codec_id == AV_CODEC_ID_MJPEG) {
                os = avformat_new_stream(oc, NULL);
                if (!os) {
                    rev = ENM4A_NO_MEMORY;
                    goto end;
                }
                if ((ret = avcodec_parameters_copy(os->codecpar, is->codecpar)) < 0) {
                    rev = ENM4A_FFMPEG_ERR;
                    goto end;
                }
                os->disposition |= AV_DISPOSITION_ATTACHED_PIC;
                os->codecpar->codec_tag = 0;
                has_img = 1;
                img_extra_file = 1;
                img_stream_index = i;
                img_dest_index = map_index++;
            }
        }
        if (!has_img) {
            avformat_close_input(&imgc);
            imgc = NULL;
        }
    }
    for (unsigned int i = 0; i < ic->nb_streams; i++) {
        AVStream* is = ic->streams[i], * os = NULL;
        if (is->codecpar->codec_id == AV_CODEC_ID_AAC) {
            os = avformat_new_stream(oc, NULL);
            if (!os) {
                rev = ENM4A_NO_MEMORY;
                goto end;
            }
            if ((ret = avcodec_parameters_copy(os->codecpar, is->codecpar)) < 0) {
                rev = ENM4A_FFMPEG_ERR;
                goto end;
            }
            os->codecpar->codec_tag = 0;
            has_audio = 1;
            audio_stream_index = i;
            audio_dest_index = map_index++;
        } else if (!has_img) {
            if (is->codecpar->codec_id == AV_CODEC_ID_MJPEG) {
                os = avformat_new_stream(oc, NULL);
                if (!os) {
                    rev = ENM4A_NO_MEMORY;
                    goto end;
                }
                if ((ret = avcodec_parameters_copy(os->codecpar, is->codecpar)) < 0) {
                    rev = ENM4A_FFMPEG_ERR;
                    goto end;
                }
                os->disposition |= AV_DISPOSITION_ATTACHED_PIC;
                os->codecpar->codec_tag = 0;
                has_img = 1;
                img_stream_index = i;
                img_dest_index = map_index++;
            }
        }
    }
    if (!has_audio) {
        rev = ENM4A_NO_AUDIO;
        goto end;
    }
    if (title) av_dict_set(&oc->metadata, "title", title, 0);
    set_ctx_metadata(oc, ic, "artist", args.artist);
    set_ctx_metadata(oc, ic, "album", args.album);
    set_ctx_metadata(oc, ic, "album_artist", args.album_artist);
    set_ctx_metadata(oc, ic, "disc", args.disc);
    set_ctx_metadata(oc, ic, "track", args.track);
    set_ctx_metadata(oc, ic, "date", args.date);
    av_dump_format(oc, 0, out, 1);
    if (!(oc->oformat->flags & AVFMT_NOFILE)) {
        if ((ret = avio_open(&oc->pb, out, AVIO_FLAG_WRITE)) < 0) {
            rev = ENM4A_ERR_OPEN_FILE;
            goto end;
        }
    }
    if ((ret = avformat_write_header(oc, NULL)) < 0) {
        rev = ENM4A_FFMPEG_ERR;
        goto end;
    }
    if (imgc && has_img && img_extra_file) {
        while (1) {
            AVStream* is = NULL, * os = NULL;
            if ((ret = av_read_frame(imgc, &pkt)) < 0) {
                if (ret == AVERROR_EOF) break;
                rev = ENM4A_FFMPEG_ERR;
                goto end;
            }
            if (pkt.data == NULL) break;
            is = imgc->streams[pkt.stream_index];
            if (pkt.stream_index != img_stream_index) {
                av_packet_unref(&pkt);
                continue;
            }
            os = oc->streams[img_dest_index];
            if (args.level >= ENM4A_LOG_TRACE) {
                log_packet(imgc, &pkt, "in");
            }
            pkt.pts = av_rescale_q_rnd(pkt.pts, is->time_base, os->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
            pkt.dts = av_rescale_q_rnd(pkt.dts, is->time_base, os->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
            pkt.duration = av_rescale_q(pkt.duration, is->time_base, os->time_base);
            pkt.pos = -1;
            pkt.stream_index = img_dest_index;
            if (args.level >= ENM4A_LOG_TRACE) {
                log_packet(oc, &pkt, "out");
            }
            if ((ret = av_interleaved_write_frame(oc, &pkt)) < 0) {
                rev = ENM4A_FFMPEG_ERR;
                goto end;
            }
            av_packet_unref(&pkt);
        }
    }
    char cn_img = has_img && !img_extra_file;
    audio_dts = INT64_MIN;
    while (1) {
        AVStream* is = NULL, * os = NULL;
        if ((ret = av_read_frame(ic, &pkt)) < 0) {
            if (ret == AVERROR_EOF) break;
            rev = ENM4A_FFMPEG_ERR;
            goto end;
        }
        if (pkt.data == NULL) break;
        is = ic->streams[pkt.stream_index];
        if (pkt.stream_index != audio_stream_index) {
            if (!cn_img || pkt.stream_index != img_stream_index) {
                av_packet_unref(&pkt);
                continue;
            }
        }
        unsigned int ind = pkt.stream_index == audio_stream_index ? audio_dest_index : img_dest_index;
        os = oc->streams[ind];
        if (args.level >= ENM4A_LOG_TRACE) {
            log_packet(ic, &pkt, "in");
        }
        pkt.pts = av_rescale_q_rnd(pkt.pts, is->time_base, os->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        pkt.dts = av_rescale_q_rnd(pkt.dts, is->time_base, os->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        if (pkt.stream_index == audio_stream_index) {
            if (audio_dts > pkt.dts) {
                av_log(NULL, AV_LOG_WARNING, "Non-monotonous DTS in output stream 0:%u; previous: %"PRId64", current: %"PRId64"; changing to %"PRId64". This may result in incorrect timestamps in the output file.\n", ind, audio_dts, pkt.dts, audio_dts + 1);
                pkt.dts = audio_dts + 1;
                pkt.pts = audio_dts + 1;
            }
            audio_dts = pkt.dts;
        }
        pkt.duration = av_rescale_q(pkt.duration, is->time_base, os->time_base);
        pkt.pos = -1;
        pkt.stream_index = ind;
        if (args.level >= ENM4A_LOG_TRACE) {
            log_packet(oc, &pkt, "out");
        }
        if (args.level <= ENM4A_LOG_DEBUG) {
#ifdef _WIN32
            GetSystemTimePreciseAsFileTime(&tnow);
            size_t ts = ft2ts(tnow) - ft2ts(pgtime);
            if (ts >= 2000000ull) {
                log_progress(oc, pkt.pts, os->time_base);
                pgtime = tnow;
            }
#elif defined(HAVE_CLOCK_GETTIME)
            if (!clock_gettime(CLOCK_REALTIME, &tnow)) {
                time_t ts = ts2ts(tnow) - ts2ts(pgtime);
                if (ts >= 200000000ll) {
                    log_progress(oc, pkt.pts, os->time_base);
                    pgtime = tnow;
                }
            }
#else
            tnow = time(NULL);
            if (pgtime < tnow) {
                log_progress(oc, pkt.pts, os->time_base);
                pgtime = tnow;
            }
#endif
        }
        if ((ret = av_interleaved_write_frame(oc, &pkt)) < 0) {
            rev = ENM4A_FFMPEG_ERR;
            goto end;
        }
        av_packet_unref(&pkt);
    }
    av_write_trailer(oc);
end:
    if (oc) {
        if (!(oc->oformat->flags & AVFMT_NOFILE)) avio_closep(&oc->pb);
        avformat_free_context(oc);
    }
    if (ic) avformat_close_input(&ic);
    if (imgc) avformat_close_input(&imgc);
    if (ret < 0 && ret != AVERROR_EOF) {
        char err[AV_ERROR_MAX_STRING_SIZE];
        printf("Error occurred: %s\n", av_make_error_string(err, AV_ERROR_MAX_STRING_SIZE, ret));
    }
    if (title) free(title);
    if (out) free(out);
    if (headers) free(headers);
    if (demux_option) {
        av_dict_free(&demux_option);
    }
    return rev;
}

const char* enm4a_error_msg(ENM4A_ERROR err) {
    switch (err)
    {
    case ENM4A_OK:
        return "OK";
    case ENM4A_NULL_POINTER:
        return "Null pointer";
    case ENM4A_NO_MEMORY:
        return "Out of memory";
    case ENM4A_FFMPEG_ERR:
        return "An error occured in ffmpeg code.";
    case ENM4A_FILE_EXISTS:
        return "Output file already exists, and overwrite is false";
    case ENM4A_ERR_REMOVE_FILE:
        return "Can not remove existed output file";
    case ENM4A_NO_AUDIO:
        return "Can not find audio stream";
    case ENM4A_ERR_OPEN_FILE:
        return "Can not open output file";
    case ENM4A_HTTP_HEADER_EMPTY_KEY:
        return "The key of HTTP HEADER is empty";
    case ENM4A_HTTP_HEADER_NO_COLON:
        return "No colon in HTTP HEADER";
    default:
        return "Unknown error";
    }
}

void enm4a_print_ffmpeg_version() {
    unsigned int v = avformat_version();
    printf("libavformat %u.%u.%u\n", v >> 16, (v & 0xff00) >> 8, v & 0xff);
    v = avcodec_version();
    printf("libavcodec %u.%u.%u\n", v >> 16, (v & 0xff00) >> 8, v & 0xff);
    v = avutil_version();
    printf("libavutil %u.%u.%u\n", v >> 16, (v & 0xff00) >> 8, v & 0xff);
}

void enm4a_print_ffmpeg_configuration() {
    printf("libavformat configuration: %s\n", avformat_configuration());
    printf("libavcodec configuration: %s\n", avcodec_configuration());
    printf("libavutil configuration: %s\n", avutil_configuration());
}

void enm4a_print_ffmpeg_license() {
    printf("libavformat licnese: %s\n", avformat_license());
    printf("libavcodec licnese: %s\n", avcodec_license());
    printf("libavutil licnese: %s\n", avutil_license());
}
