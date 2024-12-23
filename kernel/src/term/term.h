#pragma once
#include <limine.h>

#include "../flanterm/backends/fb.h"
#include "../flanterm/flanterm.h"
#define NANOPRINTF_IMPLEMENTATION
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS	 0
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS		 0
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS		 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS		 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS	 0
#define NANOPRINTF_VISIBILITY_STATIC
#include "nanoprintf.h"
void init_term(struct limine_framebuffer* buf);
__attribute__((format(printf, 1, 2))) void kprintf(const char* format, ...);
__attribute__((format(printf, 1, 2))) void sprintf(const char* format, ...);
