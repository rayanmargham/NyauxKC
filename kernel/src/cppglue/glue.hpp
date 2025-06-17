#pragma once
#include <mem/kmem.h>
#include <stddef.h>
#ifdef __cplusplus
template <typename T>
class frgalloc {
    public:
    void deallocate(T *ptr, size_t n) {
        if (!static_cast<void*>(ptr)) {
            return;
        }
        size_t *gotit = reinterpret_cast<size_t*>(reinterpret_cast<uint8_t*>(ptr) - alignof(max_align_t));
        size_t o = *gotit;
        assert(n == o);
        kfree(reinterpret_cast<void*>(gotit), o + alignof(max_align_t));
    }
    T *allocate(size_t size) {
        size_t *keepit = static_cast<size_t*>(kmalloc(size + alignof(max_align_t)));
        *keepit = size;
        return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(keepit) + alignof(max_align_t));
    }
    void free(T *ptr) {
        if (!static_cast<void*>(ptr)) {
                    return;
                }
                size_t *gotit = reinterpret_cast<size_t*>(reinterpret_cast<uint8_t*>(ptr) - alignof(max_align_t));
                size_t o = *gotit;
                kfree((reinterpret_cast<void*>(gotit)), o + alignof(max_align_t));
    }
};
extern "C" {
#endif
void frg_log(const char *cstring);
void frg_panic(const char *cstring);
#ifdef __cplusplus
}
#endif
