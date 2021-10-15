#if HAVE_ENM4A_CONFIG_H
#include "enm4a_config.h"
#endif

#include "enm4a.h"

#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>

#include "cstr_util.h"
#include "libavformat/avformat.h"
#include "libavutil/log.h"
#include "libavutil/dict.h"

#if HAVE_PRINTF_S
#define printf printf_s
#endif
#ifdef HAVE_SSCANF_S
#define sscanf sscanf_s
#endif

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
    AVFormatContext* ic = NULL, * oc = NULL;
    int ret = 0;
    ENM4A_ERROR rev = ENM4A_OK;
    if ((ret = avformat_open_input(&ic, input, NULL, NULL)) != 0) {
        rev = ENM4A_FFMPEG_ERR;
        goto end;
    }
    if ((ret = avformat_find_stream_info(ic, NULL)) != 0) {
        rev = ENM4A_FFMPEG_ERR;
        goto end;
    }
    av_dump_format(ic, 0, input, 0);
    char* title = NULL;
    if (!args.title || !strlen(args.title)) {
        if (ic->metadata) {
            AVDictionaryEntry* en = av_dict_get(ic->metadata, "title", NULL, 0);
            if (en) {
                int re = cstr_util_copy_str(&title, en->value);
                if (!re) {
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
        if (!re) {
            rev = re == 2 ? ENM4A_NO_MEMORY : ENM4A_NULL_POINTER;
            goto end;
        }
        if (args.level >= ENM4A_LOG_VERBOSE) {
            printf("Get title from input argument: %s\n", title);
        }
    }
end:
    if (ic) avformat_close_input(&ic);
    if (ret < 0 && ret != AVERROR_EOF) {
        char err[AV_ERROR_MAX_STRING_SIZE];
        printf("Error occurred: %s\n", av_make_error_string(err, AV_ERROR_MAX_STRING_SIZE, ret));
    }
    if (title) {
        free(title);
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
    default:
        return "Unknown error";
    }
}
