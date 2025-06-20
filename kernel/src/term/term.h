#pragma once

#include <limine.h>

#include "../flanterm/src/flanterm_backends/fb.h"
#include "../flanterm/src/flanterm.h"
#define NANOPRINTF_IMPLEMENTATION
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0
#define NANOPRINTF_VISIBILITY_STATIC
#include "nanoprintf.h"
#include <utils/libc.h>
#ifdef __cplusplus
extern "C" {
#endif
  void init_term(struct limine_framebuffer *buf);
  void friendinsideme(const char *format, va_list args);
  void friendinsidemewrapper(const char *format, ...);
  void reinit_term(struct limine_framebuffer *buf);
  enum LOGLEVEL { FATAL, ERROR, LOG, TRACE, STATUSOK, STATUSFAIL };
  void kprintf_log(enum LOGLEVEL, const char *format, ...);
  __attribute__((format(printf, 1, 2))) void kprintf(const char *format, ...);
  __attribute__((format(printf, 1, 2))) void sprintf(const char *format, ...);
  int is_transmit_empty();
  void sprintf_write(char *buf, size_t size);
  void sprintf_log(enum LOGLEVEL log, const char *format, ...);
  struct flanterm_context *get_fctx();
#ifdef __cplusplus
}
#endif
