#include "enm4a.h"
#include "enm4a_http_header.h"

#include <malloc.h>
#include <string.h>

ENM4A_HTTP_HEADER* enm4a_new_http_header(const char* key, const char* value, ENM4A_ERROR* err) {
    if (!key || !value) {
        if (err) *err = ENM4A_NULL_POINTER;
        return NULL;
    }
    size_t ke = strlen(key);
    size_t ve = strlen(value);
    if (ke == 0) {
        if (err) *err = ENM4A_HTTP_HEADER_EMPTY_KEY;
        return NULL;
    }
    ENM4A_HTTP_HEADER* tmp = malloc(sizeof(ENM4A_HTTP_HEADER));
    if (!tmp) {
        if (err) *err = ENM4A_NO_MEMORY;
        return NULL;
    }
    tmp->key = malloc(ke + 1);
    if (!tmp->key) {
        free(tmp);
        if (err) *err = ENM4A_NO_MEMORY;
        return NULL;
    }
    tmp->value = malloc(ve + 1);
    if (!tmp->value) {
        free(tmp->key);
        free(tmp);
        if (err) *err = ENM4A_NO_MEMORY;
        return NULL;
    }
    memcpy(tmp->key, key, ke);
    tmp->key[ke] = 0;
    memcpy(tmp->value, value, ve);
    tmp->value[ve] = 0;
    if (err) *err = ENM4A_OK;
    return tmp;
}

void enm4a_free_http_header(ENM4A_HTTP_HEADER* h) {
    if (!h) return;
    if (h->key) free(h->key);
    if (h->value) free(h->value);
    free(h);
}

ENM4A_HTTP_HEADER* enm4a_parse_http_header(const char* inp, ENM4A_ERROR* err) {
    if (!inp) {
        if (err) *err = ENM4A_NULL_POINTER;
        return NULL;
    }
    char* c = strchr(inp, ':');
    if (c == NULL) {
        if (err) *err = ENM4A_HTTP_HEADER_NO_COLON;
        return NULL;
    }
    size_t te = strlen(inp);
    size_t ke = c - inp;
    size_t ve = te - ke - 1;
    if (ke == 0) {
        if (err) *err = ENM4A_HTTP_HEADER_EMPTY_KEY;
        return NULL;
    }
    char* key = malloc(ke + 1);
    if (!key) {
        if (err) *err = ENM4A_NO_MEMORY;
        return NULL;
    }
    char* value = malloc(ve + 1);
    if (!value) {
        free(key);
        if (err) *err = ENM4A_NO_MEMORY;
        return NULL;
    }
    memcpy(key, inp, ke);
    key[ke] = 0;
    memcpy(value, c + 1, ve);
    value[ve] = 0;
    ENM4A_HTTP_HEADER* ret = enm4a_new_http_header(key, value, err);
    free(key);
    free(value);
    return ret;
}

char* enm4a_generate_http_header(ENM4A_HTTP_HEADER** list, size_t len, ENM4A_ERROR* err) {
    if (!list) {
        if (err) *err = ENM4A_NULL_POINTER;
        return NULL;
    }
    size_t i = 0;
    size_t buf_len = 0;
    for (i = 0; i < len; i++) {
        if (!list[i]) {
            if (err) *err = ENM4A_NULL_POINTER;
            return NULL;
        }
        buf_len += strlen(list[i]->key) + strlen(list[i]->value) + 3;
    }
    char* tmp = malloc(buf_len + 1);
    if (!tmp) {
        if (err) *err = ENM4A_NO_MEMORY;
        return NULL;
    }
    char* ntmp = tmp;
    size_t le = 0;
    for (i = 0; i < len; i++) {
        le = strlen(list[i]->key);
        memcpy(ntmp, list[i]->key, le);
        ntmp += le;
        *ntmp = ':';
        ntmp++;
        le = strlen(list[i]->value);
        memcpy(ntmp, list[i]->value, le);
        ntmp += le;
        memcpy(ntmp, "\r\n", 2);
        ntmp += 2;
    }
    tmp[buf_len] = 0;
    return tmp;
}
