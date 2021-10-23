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
#include "libavutil/audio_fifo.h"
#include "libavutil/channel_layout.h"
#include "libswresample/swresample.h"

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

const AVCodec* find_aac_codec_encoder() {
    const AVCodec* c = avcodec_find_encoder(AV_CODEC_ID_AAC);
    return c;
}

ENM4A_ERROR enm4a_is_supported_sample_rates(int sample_rate, int* result) {
    if (!result) return ENM4A_NULL_POINTER;
    const AVCodec* c = find_aac_codec_encoder();
    if (!c) return ENM4A_NO_ENCODER;
    int i = 0;
    while (c->supported_samplerates[i] != 0) {
        if (sample_rate == c->supported_samplerates[i]) {
            *result = 1;
            return ENM4A_OK;
        }
        i++;
    }
    *result = 0;
    return ENM4A_OK;
}

void init_enm4a_args(ENM4A_ARGS* args) {
    if (!args) return;
    memset(args, 0, sizeof(ENM4A_ARGS));
    args->default_sample_rate = 48000;
    args->bitrate = 320 * 1000;
}

void set_audio_samplerate(const AVCodecContext* in, AVCodecContext* out, const AVCodec* oc, int default_sample_rate, ENM4A_ERROR* err) {
    if (!in || !out || !oc) {
        if (err) *err = ENM4A_NULL_POINTER;
        return;
    }
    int sr = in->sample_rate;
    int i = 0;
    while (oc->supported_samplerates[i] != 0) {
        if (sr == oc->supported_samplerates[i]) {
            out->sample_rate = sr;
            if (err) *err = ENM4A_OK;
            return;
        }
        i++;
    }
    out->sample_rate = default_sample_rate;
    if (err) *err = ENM4A_OK;
}

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
    printf("%s\r", buf);
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

ENM4A_ERROR convert_samples_and_add_to_fifo(int* ret, AVCodecContext* out, SwrContext* sw, AVFrame* frame, AVAudioFifo* fifo) {
    if (!ret || !out || !sw || !frame || !fifo) return ENM4A_NULL_POINTER;
    uint8_t** converted_input_samples = NULL;
    ENM4A_ERROR re = ENM4A_OK;
    if (!(converted_input_samples = malloc(sizeof(void*) * out->channels))) {
        re = ENM4A_NO_MEMORY;
        goto end;
    }
    if ((*ret = av_samples_alloc(converted_input_samples, NULL, out->channels, frame->nb_samples, out->sample_fmt, 0)) < 0) {
        re = ENM4A_NO_MEMORY;
        goto end;
    }
    if ((*ret = swr_convert(sw, converted_input_samples, frame->nb_samples, frame->extended_data, frame->nb_samples)) < 0) {
        re = ENM4A_FFMPEG_ERR;
        goto end;
    }
    if ((*ret = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frame->nb_samples)) < 0) {
        re = ENM4A_NO_MEMORY;
        goto end;
    }
    if (av_audio_fifo_write(fifo, (void**)converted_input_samples, frame->nb_samples) < frame->nb_samples) {
        re = ENM4A_FIFO_WRITE_ERR;
        goto end;
    }
end:
    if (converted_input_samples) {
        av_freep(&converted_input_samples[0]);
        free(converted_input_samples);
    }
    return re;
}

ENM4A_ERROR encode_audio_frame(int* ret, AVFrame* frame, AVFormatContext* oc, AVCodecContext* occ, char* writed_data, int64_t* pts, ENM4A_LOG level) {
    if (!oc || !occ || !ret) return ENM4A_NULL_POINTER;
    if (frame && !pts) return ENM4A_NULL_POINTER;
    ENM4A_ERROR re = ENM4A_OK;
    *writed_data = 0;
    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        re = ENM4A_NO_MEMORY;
        goto end;
    }
    if (frame) {
        frame->pts = *pts;
        *pts += frame->nb_samples;
    }
    if ((*ret = avcodec_send_frame(occ, frame)) < 0) {
        if (*ret == AVERROR_EOF) {
            *ret = 0;
        } else {
            re = ENM4A_FFMPEG_ERR;
            goto end;
        }
    }
    *ret = avcodec_receive_packet(occ, pkt);
    if (*ret >= 0) {
        *writed_data = 1;
    } else if (*ret == AVERROR_EOF) {
        *ret = 0;
        goto end;
    } else if (*ret == AVERROR(EAGAIN)) {
        *ret = 0;
        *writed_data = 1;
        goto end;
    } else {
        re = ENM4A_FFMPEG_ERR;
        goto end;
    }
    if (*writed_data && (*ret = av_write_frame(oc, pkt)) < 0) {
        re = ENM4A_FFMPEG_ERR;
        goto end;
    }
    if (level >= ENM4A_LOG_TRACE) {
        log_packet(oc, pkt, "out");
    }
end:
    if (pkt) {
        av_packet_free(&pkt);
    }
    return re;
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
    int is_supported;
    AVFormatContext* ic = NULL, * oc = NULL, * imgc = NULL;
    int ret = 0;
    ENM4A_ERROR rev = ENM4A_OK;
    char* title = NULL, * out = NULL, * headers = NULL;
    char has_img = 0, img_extra_file = 0, has_audio = 0, audio_need_encode = 0;
    unsigned int img_stream_index = 0, audio_stream_index = 0, img_dest_index = 0, audio_dest_index = 0, map_index = 0;
    AVPacket pkt;
    int64_t audio_dts, audio_pts = 0;
    AVDictionary* demux_option = NULL;
    AVCodecContext* audio_input = NULL, * audio_output = NULL;
    SwrContext* resample_context = NULL;
    AVAudioFifo* afifo = NULL;
    AVFrame* audio_input_frame = NULL, * audio_output_frame = NULL;
#ifdef _WIN32
    FILETIME pgtime = { 0, 0 }, tnow = { 0, 0 };
#elif defined(HAVE_CLOCK_GETTIME)
    struct timespec pgtime = { LLONG_MIN, 0 }, tnow = { 0, 0 };
#else
    time_t pgtime = LLONG_MIN, tnow = 0;
#endif
    if ((rev = enm4a_is_supported_sample_rates(args.default_sample_rate, &is_supported)) != ENM4A_OK) {
        goto end;
    }
    if (!is_supported) {
        rev = ENM4A_INVALID_DEFUALE_SAMPLE_RATE;
        goto end;
    }
    if (args.sample_rate) {
        if ((rev = enm4a_is_supported_sample_rates(*(args.sample_rate), &is_supported)) != ENM4A_OK) {
            goto end;
        }
        if (!is_supported) {
            rev = ENM4A_INVALID_SAMPLE_RATE;
            goto end;
        }
    }
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
            if (is->codecpar->codec_id == AV_CODEC_ID_MJPEG && !has_img) {
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
                break;
            }
        }
        if (!has_img) {
            avformat_close_input(&imgc);
            imgc = NULL;
        }
    }
    for (unsigned int i = 0; i < ic->nb_streams; i++) {
        AVStream* is = ic->streams[i], * os = NULL;
        if (is->codecpar->codec_id == AV_CODEC_ID_AAC && !has_audio) {
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
        for (unsigned int i = 0; i < ic->nb_streams; i++) {
            AVStream* is = ic->streams[i], * os = NULL;
            if (is->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && !has_audio) {
                const AVCodec* input_codec = avcodec_find_decoder(is->codecpar->codec_id), * output_codec = NULL;
                if (!input_codec) {
                    rev = ENM4A_NO_DECODER;
                    goto end;
                }
                audio_input = avcodec_alloc_context3(input_codec);
                if (!audio_input) {
                    rev = ENM4A_NO_MEMORY;
                    goto end;
                }
                if ((ret = avcodec_parameters_to_context(audio_input, is->codecpar)) < 0) {
                    rev = ENM4A_FFMPEG_ERR;
                    goto end;
                }
                if ((ret = avcodec_open2(audio_input, input_codec, NULL)) < 0) {
                    rev = ENM4A_FFMPEG_ERR;
                    goto end;
                }
                if (!(output_codec = find_aac_codec_encoder())) {
                    rev = ENM4A_NO_ENCODER;
                    goto end;
                }
                if (!(os = avformat_new_stream(oc, NULL))) {
                    rev = ENM4A_NO_MEMORY;
                    goto end;
                }
                if (!(audio_output = avcodec_alloc_context3(output_codec))) {
                    rev = ENM4A_NO_MEMORY;
                    goto end;
                }
                audio_output->channels = audio_input->channels;
                audio_output->channel_layout = av_get_default_channel_layout(audio_output->channels);
                if (args.sample_rate) {
                    audio_output->sample_rate = *(args.sample_rate);
                } else {
                    set_audio_samplerate(audio_input, audio_output, output_codec, args.default_sample_rate, &rev);
                }
                if (rev != ENM4A_OK) {
                    goto end;
                }
                audio_output->sample_fmt = output_codec->sample_fmts[0];
                audio_output->bit_rate = args.bitrate;
                os->time_base.den = audio_output->sample_rate;
                os->time_base.num = 1;
                if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
                    audio_output->flags |= AVFMT_GLOBALHEADER;
                }
                if ((ret = avcodec_open2(audio_output, output_codec, NULL)) < 0) {
                    rev = ENM4A_FFMPEG_ERR;
                    goto end;
                }
                if ((ret = avcodec_parameters_from_context(os->codecpar, audio_output)) < 0) {
                    rev = ENM4A_FFMPEG_ERR;
                    goto end;
                }
                resample_context = swr_alloc_set_opts(NULL, av_get_default_channel_layout(audio_output->channels), audio_output->sample_fmt, audio_output->sample_rate, av_get_default_channel_layout(audio_input->channels), audio_input->sample_fmt, audio_input->sample_rate, 0, NULL);
                if (!resample_context) {
                    rev = ENM4A_NO_MEMORY;
                    goto end;
                }
                if ((ret = swr_init(resample_context)) < 0) {
                    rev = ENM4A_FFMPEG_ERR;
                    goto end;
                }
                if (!(afifo = av_audio_fifo_alloc(audio_output->sample_fmt, audio_output->channels, 1))) {
                    rev = ENM4A_NO_MEMORY;
                    goto end;
                }
                has_audio = 1;
                audio_stream_index = i;
                audio_dest_index = map_index++;
                audio_need_encode = 1;
                break;
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
    if (audio_need_encode) {
        if (!(audio_input_frame = av_frame_alloc())) {
            rev = ENM4A_NO_MEMORY;
            goto end;
        }
        if (!(audio_output_frame = av_frame_alloc())) {
            rev = ENM4A_NO_MEMORY;
            goto end;
        }
        audio_output_frame->channel_layout = audio_output->channel_layout;
        audio_output_frame->format = audio_output->sample_fmt;
        audio_output_frame->sample_rate = audio_output->sample_rate;
    }
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
    char cn_img = has_img && !img_extra_file, finished = 0, write_data = 1;
    audio_dts = INT64_MIN;
    while (1) {
        AVStream* is = NULL, * os = NULL;
        if ((ret = av_read_frame(ic, &pkt)) < 0) {
            if (ret == AVERROR_EOF) {
                finished = 1;
                if (!audio_need_encode) break;
            } else {
                rev = ENM4A_FFMPEG_ERR;
                goto end;
            }
        }
        if (pkt.data == NULL) {
            finished = 1;
            if (!audio_need_encode) break;
        }
        if (!finished) is = ic->streams[pkt.stream_index];
        if (!finished && pkt.stream_index != audio_stream_index) {
            if (!cn_img || pkt.stream_index != img_stream_index) {
                av_packet_unref(&pkt);
                continue;
            }
        }
        char is_audio = !finished ? pkt.stream_index == audio_stream_index : 0;
        unsigned int ind = is_audio ? audio_dest_index : img_dest_index;
        if (!finished) os = oc->streams[ind];
        if (args.level >= ENM4A_LOG_TRACE && !finished) {
            log_packet(ic, &pkt, "in");
        }
        if ((is_audio && audio_need_encode) || finished) {
            if (!finished) {
                if ((ret = avcodec_send_packet(audio_input, &pkt)) < 0) {
                    rev = ENM4A_FFMPEG_ERR;
                    goto end;
                }
                ret = avcodec_receive_frame(audio_input, audio_input_frame);
                if (ret >= 0) {
                    if ((rev = convert_samples_and_add_to_fifo(&ret, audio_output, resample_context, audio_input_frame, afifo)) != ENM4A_OK) {
                        goto end;
                    }
                }
                else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    ret = 0;
                } else {
                    rev = ENM4A_FFMPEG_ERR;
                    goto end;
                }
            }
            if (args.level >= ENM4A_LOG_TRACE) {
                printf("the size of fifo: %d\n", av_audio_fifo_size(afifo));
            }
            while (av_audio_fifo_size(afifo) >= audio_output->frame_size || (finished && av_audio_fifo_size(afifo) > 0)) {
                audio_output_frame->nb_samples = FFMIN(av_audio_fifo_size(afifo), audio_output->frame_size);
                if ((ret = av_frame_get_buffer(audio_output_frame, 0)) < 0) {
                    rev = ENM4A_NO_MEMORY;
                    goto end;
                }
                if ((ret = av_audio_fifo_read(afifo, audio_output_frame->data, audio_output_frame->nb_samples)) < 0) {
                    rev = ENM4A_NO_MEMORY;
                    goto end;
                }
                if ((rev = encode_audio_frame(&ret, audio_output_frame, oc, audio_output, &write_data, &audio_pts, args.level)) != ENM4A_OK) {
                    goto end;
                }
                if (args.level >= ENM4A_LOG_TRACE) {
                    printf("the size of fifo: %d\n", av_audio_fifo_size(afifo));
                }
            }
            if (finished) {
                while (1) {
                    if ((rev = encode_audio_frame(&ret, NULL, oc, audio_output, &write_data, NULL, args.level)) != ENM4A_OK) {
                        goto end;
                    }
                    if (!write_data) break;
                }
            }
        } else {
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
            write_data = 1;
            if (args.level >= ENM4A_LOG_TRACE) {
                log_packet(oc, &pkt, "out");
            }
            if ((ret = av_interleaved_write_frame(oc, &pkt)) < 0) {
                rev = ENM4A_FFMPEG_ERR;
                goto end;
            }
        }
        if (args.level <= ENM4A_LOG_DEBUG) {
#ifdef _WIN32
            GetSystemTimePreciseAsFileTime(&tnow);
            size_t ts = ft2ts(tnow) - ft2ts(pgtime);
            if (ts >= 2000000ull) {
                if (os) log_progress(oc, pkt.pts, os->time_base);
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
        if (!finished) av_packet_unref(&pkt);
        if (finished) break;
    }
    av_write_trailer(oc);
end:
    if (audio_output_frame) {
        av_frame_free(&audio_output_frame);
    }
    if (audio_input_frame) {
        av_frame_free(&audio_input_frame);
    }
    if (afifo) {
        av_audio_fifo_free(afifo);
    }
    if (resample_context) {
        swr_free(&resample_context);
    }
    if (audio_input) {
        avcodec_free_context(&audio_input);
    }
    if (audio_output) {
        avcodec_free_context(&audio_output);
    }
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
    case ENM4A_NO_DECODER:
        return "No suitable decoder finded.";
    case ENM4A_NO_ENCODER:
        return "Can not find ACC encoder.";
    case ENM4A_INVALID_DEFUALE_SAMPLE_RATE:
        return "Unsupported default sample rate.";
    case ENM4A_INVALID_SAMPLE_RATE:
        return "Unsupported sample rate.";
    case ENM4A_FIFO_WRITE_ERR:
        return "Can not write data to FIFO.";
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
