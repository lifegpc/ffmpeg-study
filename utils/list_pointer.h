#ifndef _UTIL_LIST_POINTER_H
#define _UTIL_LIST_POINTER_H
#include <list>
#include <malloc.h>

template <typename T, typename R>
bool listToPointer(std::list<T> li, R*& result, bool (*convert)(T input, R& output), void (*free)(R input)) {
    if (!convert) return false;
    auto sz = li.size();
    if (sz <= 0) return false;
    R* r = (R*)malloc(sz * sizeof(void*));
    if (!r) {
        return false;
    }
    size_t j = 0;
    for (auto i = li.begin(); i != li.end(); ++i) {
        auto s = *i;
        R t;
        if (!convert(s, t)) {
            if (!free) {
                for (size_t z = 0; z < j; z++) {
                    free(r[z]);
                }
            }
            ::free(r);
            return false;
        }
        r[j] = t;
        j++;
    }
    result = r;
    return true;
}

template <typename T>
void freePointerList(T* li, size_t count, void (*free)(T)) {
    if (!li) return;
    for (size_t i = 0; i < count; i++) {
        if (!free) ::free(li[i]);
        else free(li[i]);
    }
    ::free(li);
}
#endif
