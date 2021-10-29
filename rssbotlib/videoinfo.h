#ifndef _RSSBOTLIB_VIDEOINFO_H
#define _RSSBOTLIB_VIDEOINFO_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavutil/dict.h"
typedef enum VideoInfoType {
    VIDEOINFO_OK,
    VIDEOINFO_FFMPEG_ERROR,
    VIDEOINFO_NULL_POINTER,
    VIDEOINFO_NO_MEMORY,
} VideoInfoType;

typedef struct VideoInfoError {
    VideoInfoType typ;
    int fferr;
} VideoInfoError;

typedef struct CStreamInfo {
    int index;
    int id;
    AVRational time_base;
    int64_t duration;
    AVDictionary* metadata;
    AVCodecParameters* codecpar;
} CStreamInfo;

typedef struct CVideoInfo {
    char ok;
    AVDictionary* meta;
    int64_t duration;
    const char* mime_type;
    const char* type_name;
    const char* type_long_name;
    unsigned int nb_chapters;
    AVChapter** chapters;
    unsigned int nb_streams;
    CStreamInfo** streams;
} CVideoInfo;

void init_avchapter(AVChapter* ch);
void init_streaminfo(CStreamInfo* info);
void init_videoinfo(CVideoInfo* info);
VideoInfoError parse_videoinfo(CVideoInfo* info, char* url);
void free_streaminfo(CStreamInfo* info);
void free_videoinfo(CVideoInfo* info);
void init_videoinfo_error(VideoInfoError* err);
const char* videoinfo_err_msg(VideoInfoError err);

#ifdef __cplusplus
}
#endif
#endif
