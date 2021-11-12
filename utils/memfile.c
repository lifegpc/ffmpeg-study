#include "memfile.h"
#include <malloc.h>
#include <string.h>
#include <errno.h>

#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

MemFile* new_memfile(const char* data, size_t len) {
    if (!data || !len) return NULL;
    MemFile* f = malloc(sizeof(MemFile));
    if (!f) return NULL;
    memset(f, 0, sizeof(MemFile));
    f->data = malloc(len);
    if (!f->data) {
        free(f);
        return NULL;
    }
    memcpy(f->data, data, len);
    f->len = len;
    f->loc = 0;
    return f;
}

void free_memfile(MemFile* f) {
    if (!f) return;
    if (f->data) {
        free(f->data);
    }
    free(f);
}

size_t memfile_read(MemFile* f, char* buf, size_t buf_len) {
    if (!f || !buf) return (size_t)-1;
    if (!buf_len || f->loc >= f->len) return 0;
    size_t le = min(buf_len, f->len - f->loc);
    memcpy(buf, f->data + f->loc, le);
    f->loc += le;
    return le;
}

#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#define FFERRTAG(a, b, c, d) (-(int)MKTAG(a, b, c, d))
#define AVERROR_EOF FFERRTAG( 'E','O','F',' ')
#define AVERROR(e) (-(e))

int memfile_readpacket(void* f, uint8_t* buf, int buf_size) {
    size_t ret = memfile_read((MemFile*)f, (char*)buf, buf_size);
    if (ret == 0) return AVERROR(EINVAL);
    else if (ret == (size_t)-1) return -1;
    else return ret;
}
