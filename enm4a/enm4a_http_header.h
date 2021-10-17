#ifndef _ENM4A_ENM4A_HTTP_HEADER_H
#define _ENM4A_ENM4A_HTTP_HEADER_H

#ifdef __cplusplus
extern "C" {
#endif
typedef struct ENM4A_HTTP_HEADER {
    char* key;
    char* value;
} ENM4A_HTTP_HEADER;

/**
 * @brief Generate HTTP Headers
 * @param list The pointer of a list of HTTP Headers
 * @param len The list length
 * @return NULL if error occured. Need free memory by calling free.
*/
char* enm4a_generate_http_header(ENM4A_HTTP_HEADER** list, size_t len, ENM4A_ERROR* err);
#ifdef __cplusplus
}
#endif

#endif
