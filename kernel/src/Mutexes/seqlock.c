#include "seqlock.h"

void seq_read(struct seq_lock *lock, void (*read)(void *data, void *variable), void *data, void *variable) {
    for (;;) {
        // acquire prevents do_read() from being reordered before it
        size_t version = __atomic_load_n(&lock->version, __ATOMIC_ACQUIRE);
        if (version & 1) {
            // odd is update in progress dont do anything and keep checking 
            #ifdef __x86_64__
            // monkuous said this
            // "it's not necessary but it helps with hyper-threading performance, 
            // hypervisor performance, and (supposedly) power usage"
            // so i think we should do this
            __asm__ volatile ("pause");
            #endif
            continue;
        }
        read(data, variable);
        // this turns read() into one giant acquire op, 
        // preventing the version load from being reordered before it
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        // this checks if the variable got updated again after we did ze read
        // so we gotta retry lol
        if (__atomic_load_n(&lock->version, __ATOMIC_RELAXED) == version) break;
    }
}
void seq_write_mutiwriters(struct seq_lock *lock, void (*write)(void *data, void *variable), void *data, void *variable) {
    size_t version = __atomic_load_n(&lock->version, __ATOMIC_RELAXED);
    // relaxed is fine here,
    // we don't care about stuff before this being reordered after it
    for (;;) {
        if (version & 1) {
             // odd is update in progress dont do anything and keep checking 
            #ifdef __x86_64__
            // monkuous said this
            // "it's not necessary but it helps with hyper-threading performance, 
            // hypervisor performance, and (supposedly) power usage"
            // so i think we should do this
            __asm__ volatile ("pause");
            #endif
            continue;
        }
        if (__atomic_compare_exchange_n(&lock->version, 
            &version, version + 1, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
                // cmpxchg is (partially) a load, so it can be acquire - 
                // which means we do not need any fences to prevent do_write() 
                // from being reordered before it
                // also sets version to updating
            break;
        }
    }
    write(data, variable);
    // remember even = done updating, odd = updating
    // set version to done updating
    // release prevents do_write() from being reordered after it
    __atomic_store_n(&lock->version, version + 2, __ATOMIC_RELEASE); 
    
}
void seq_write(struct seq_lock *lock, void (*write)(void *variable, void *data), void *data, void *variable) {
    // relaxed is fine here,
    // we don't care about stuff before this being reordered after it
    size_t version = __atomic_load_n(&lock->version, __ATOMIC_RELAXED);
    __atomic_store_n(&lock->version, version + 1, __ATOMIC_RELAXED);
    __atomic_thread_fence(__ATOMIC_RELEASE);
    // this turns do_write() into one giant release op, 
    // preventing the version store from being reordered after
    write(data, variable);
    // remember even = done updating, odd = updating
    // set version to done updating
    // release prevents do_write() from being reordered after it
    __atomic_store_n(&lock->version, version + 2, __ATOMIC_RELEASE);

}