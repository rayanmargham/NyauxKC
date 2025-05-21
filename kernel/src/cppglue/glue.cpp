#include "glue.hpp"

void *operator new(unsigned long amount) { return kmalloc(amount); }