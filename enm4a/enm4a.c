#if HAVE_ENM4A_CONFIG_H
#include "enm4a_config.h"
#endif

#include "enm4a.h"

#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>

#include "libavformat/avformat.h"
#include "libavutil/log.h"

#ifdef HAVE_SSCANF_S
#define sscanf sscanf_s
#endif

typedef struct Enm4aNoInTotal {
    size_t no;
    size_t total;
} Enm4aNoInTotal;

ENM4A_ERROR parse_no_in_total(Enm4aNoInTotal* dest, const char* s) {
    if (!dest) return ENM4A_NULL_POINTER;
    size_t olen = strlen(s);
    char* temp = malloc(olen + 1);
    if (!temp) return ENM4A_NO_MEMORY;
    size_t dlen = 0;
    for (size_t i = 0; i < olen; i++) {
        if (s[i] != ' ') {
            temp[dlen++] = s[i];
        }
    }
    temp[dlen] = 0;
    size_t n = 0, t = 0;
    int re = sscanf(temp, "%zi/%zi", &n, &t);
    free(temp);
    if (re != 2 || n > t) return ENM4A_INVALID_NO_IN_TOTAL;
    dest->no = n;
    dest->total = t;
    return ENM4A_OK;
}

ENM4A_ERROR new_no_in_total(Enm4aNoInTotal* dest, size_t no, size_t total) {
    if (!dest) return ENM4A_NULL_POINTER;
    if (no > total) return ENM4A_INVALID_NO_IN_TOTAL;
    dest->no = no;
    dest->total = total;
    return ENM4A_OK;
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
end:
    if (ic) avformat_close_input(&ic);
    if (ret < 0 && ret != AVERROR_EOF) {
        char err[AV_ERROR_MAX_STRING_SIZE];
        printf("Error occurred: %s\n", av_make_error_string(err, AV_ERROR_MAX_STRING_SIZE, ret));
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
        return "Don't have enough memory.";
    case ENM4A_INVALID_NO_IN_TOTAL:
        return "No is greater than total.";
    case ENM4A_FFMPEG_ERR:
        return "An error occured in ffmpeg code.";
    default:
        return "Unknown error";
    }
}
