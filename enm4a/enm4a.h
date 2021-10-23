#ifndef _ENM4A_ENM4A_H
#define _ENM4A_ENM4A_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stddef.h>

typedef enum ENM4A_ERROR {
    ENM4A_OK,
    ENM4A_NULL_POINTER,
    ENM4A_NO_MEMORY,
    ENM4A_FFMPEG_ERR,
    ENM4A_FILE_EXISTS,
    ENM4A_ERR_REMOVE_FILE,
    ENM4A_NO_AUDIO,
    ENM4A_ERR_OPEN_FILE,
    ENM4A_HTTP_HEADER_EMPTY_KEY,
    ENM4A_HTTP_HEADER_NO_COLON,
    ENM4A_NO_DECODER,
    ENM4A_NO_ENCODER,
    ENM4A_INVALID_DEFUALE_SAMPLE_RATE,
    ENM4A_INVALID_SAMPLE_RATE,
    ENM4A_FIFO_WRITE_ERR,
} ENM4A_ERROR;

typedef enum ENM4A_LOG {
    ENM4A_LOG_INFO,
    ENM4A_LOG_VERBOSE,
    ENM4A_LOG_DEBUG,
    ENM4A_LOG_TRACE,
} ENM4A_LOG;

typedef enum ENM4A_OVERWRITE {
    ENM4A_OVERWRITE_ASK,
    ENM4A_OVERWRITE_YES,
    ENM4A_OVERWRITE_NO,
} ENM4A_OVERWRITE;

typedef struct ENM4A_HTTP_HEADER ENM4A_HTTP_HEADER;

/// Call init_enm4a_args to initialize
typedef struct ENM4A_ARGS {
    ENM4A_LOG level;
    ENM4A_OVERWRITE overwrite;
    char* output;
    char* title;
    char* artist;
    char* album;
    char* album_artist;
    char* cover;
    char* disc;
    char* track;
    char* date;
    /// A pointer to a list of HTTP header
    ENM4A_HTTP_HEADER** http_headers;
    /// The length of the list of HTTP headers
    size_t http_header_size;
    int default_sample_rate;
    int* sample_rate;
    /// Target bitrate. Used when need encoding.
    int64_t bitrate;
    /// Print log level
    char print_level;
} ENM4A_ARGS;

/**
 * @brief Check sample rate is supported by AAC encoder
 * @param sample_rate Sample rate
 * @param result Result. 0 if not supported.
 * @return ENM4A_OK if successed. If error occured. result will not set
*/
ENM4A_ERROR enm4a_is_supported_sample_rates(int sample_rate, int* result);
void init_enm4a_args(ENM4A_ARGS* args);
ENM4A_ERROR encode_m4a(const char* input, ENM4A_ARGS args);
const char* enm4a_error_msg(ENM4A_ERROR err);
void enm4a_print_ffmpeg_version();
void enm4a_print_ffmpeg_configuration();
void enm4a_print_ffmpeg_license();
ENM4A_HTTP_HEADER* enm4a_new_http_header(const char* key, const char* value, ENM4A_ERROR* err);
void enm4a_free_http_header(ENM4A_HTTP_HEADER* h);
/**
 * @brief Parse HTTP Header from string
 * @param inp Input string. eg. key: value.
 * @param err Error details. Can be NULL.
 * @return NULL if error occured. See err to get detailed information.
*/
ENM4A_HTTP_HEADER* enm4a_parse_http_header(const char* inp, ENM4A_ERROR* err);
#ifdef __cplusplus
}
#endif

#endif
