#include "term.h"
#include "utils/basic.h"
#include <stdint.h>
struct flanterm_context *ft_ctx = NULL;

void init_term(struct limine_framebuffer *buf) {
  ft_ctx = flanterm_fb_init(
      NULL, NULL, buf->address, buf->width, buf->height, buf->pitch,
      buf->blue_mask_size, buf->red_mask_shift, buf->green_mask_size,
      buf->green_mask_shift, buf->blue_mask_size, buf->blue_mask_shift, NULL,
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 1, 0, 0, 50);
}

void tputc(int ch, void *) {
  if (ft_ctx == NULL) {
    return;
  }
  char c = ch;
  flanterm_write(ft_ctx, &c, 1);
  outb(0x3F8, (uint8_t)c);
}
void kprintf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  npf_vpprintf(tputc, NULL, format, args);
  va_end(args);
}
