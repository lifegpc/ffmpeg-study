#include "utils.h"

#include <stddef.h>

int is_supported_pixfmt(enum AVPixelFormat fmt, const enum AVPixelFormat* fmts) {
    size_t i = 0;
    if (fmt == AV_PIX_FMT_NONE || !fmts) return 0;
    while (fmts[i] != AV_PIX_FMT_NONE) {
        if (fmt == fmts[i]) return 1;
        i++;
    }
    return 0;
}
