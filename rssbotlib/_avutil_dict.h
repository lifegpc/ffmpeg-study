#include "libavutil/dict.h"

// A hack, defined in libavutil/dict.c
typedef struct AVDictionary {
    int count;
    AVDictionaryEntry *elems;
} AVDictionary;
