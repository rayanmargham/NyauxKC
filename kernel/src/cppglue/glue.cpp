#include "glue.hpp"
#include <frg/list.hpp>
void *operator new(unsigned long amount) { return kmalloc(amount); }

// void operator delete() {

// }
void operator delete(void *ptr, unsigned long length) { kfree(ptr, length); }

void frg_log(const char *cstring) {
   kprintf("frg: %s\r\n", cstring);
}
void frg_panic(const char *cstring)  {
   panic("frg: %s", cstring);
}
