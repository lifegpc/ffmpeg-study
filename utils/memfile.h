#ifndef _UTIL_MEMFILE_H
#define _UTIL_MEMFILE_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <stdint.h>
typedef struct MemFile {
    char* data;
    size_t len;
    size_t loc;
} MemFile;

/**
 * @brief Create a new memory file
 * @param data Data. Will allocate new memory for this data.
 * @param len The size of data.
 * @return MemFile struct if succeessed otherwise NULL.
*/
MemFile* new_memfile(const char* data, size_t len);
void free_memfile(MemFile* f);
size_t memfile_read(MemFile* f, char* buf, size_t buf_len);
int memfile_readpacket(void* f, uint8_t* buf, int buf_size);
#ifdef __cplusplus
}
#endif
#endif
