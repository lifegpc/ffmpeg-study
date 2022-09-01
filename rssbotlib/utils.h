#ifndef _RSSBOTLIB_UTILS_H
#define _RSSBOTLIB_UTILS_H
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/pixfmt.h"
int is_supported_pixfmt(enum AVPixelFormat fmt, const enum AVPixelFormat* fmts);
#ifdef __cplusplus
}
#endif
#endif
