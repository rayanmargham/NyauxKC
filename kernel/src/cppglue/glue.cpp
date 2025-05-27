#include "glue.hpp"

void *operator new(unsigned long amount) { return kmalloc(amount); }

// void operator delete() {

// }
void operator delete(void *ptr, unsigned long length) { kfree(ptr, length); }