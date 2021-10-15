#if HAVE_ENM4A_CONFIG_H
#include "enm4a_config.h"
#endif

#include "enm4a.h"

#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>

#include "cstr_util.h"
#include "cfileop.h"
#include "libavformat/avformat.h"
#include "libavutil/log.h"
#include "libavutil/dict.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif
#ifdef HAVE_SSCANF_S
#define sscanf sscanf_s
#endif

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
    char* title = NULL, * out = NULL;
    char has_img = 0, img_extra_file = 0, has_audio = 0;
    unsigned int img_stream_index = 0, audio_stream_index = 0;
    if ((ret = avformat_open_input(&ic, input, NULL, NULL)) != 0) {
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
            size_t le = strlen(title);
            out = malloc(le + 5);
            if (!out) {
                rev = ENM4A_NO_MEMORY;
                goto end;
            }
            memcpy(out, title, le);
            memcpy(out + le, ".m4a", 4);
            out[le + 4] = 0;
            if (args.level >= ENM4A_LOG_VERBOSE) {
                printf("Get output filename from title: %s\n", out);
            }
        } else {
            out = malloc(6);
            if (!out) {
                rev = ENM4A_NO_MEMORY;
                goto end;
            }
            memcpy(out, "a.m4a", 5);
            out[5] = 0;
            if (args.level >= ENM4A_LOG_VERBOSE) {
                printf("Use default output filename: %s\n", out);
            }
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
    default:
        return "Unknown error";
    }
}
