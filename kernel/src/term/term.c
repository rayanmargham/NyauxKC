#include "term.h"

#include <arch/x86_64/instructions/instructions.h>
#include <stdint.h>

#include "mem/kmem.h"
#include "term/term.h"
#include "utils/basic.h"

struct flanterm_context *ft_ctx = NULL;
spinlock_t lock = 0;
void stolen_osdevwikiserialinit();

void *flanterm_fb_alloc(size_t size) { return kmalloc(size); }
void flanterm_fb_free(void *ptr) { kfree(ptr, 8); }

void init_term(struct limine_framebuffer *buf) {
  ft_ctx = flanterm_fb_init(
      flanterm_fb_alloc, flanterm_fb_free, buf->address, buf->width,
      buf->height, buf->pitch, buf->blue_mask_size, buf->red_mask_shift,
      buf->green_mask_size, buf->green_mask_shift, buf->blue_mask_size,
      buf->blue_mask_shift, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0,
      0, 1, 0, 0, 0);
  stolen_osdevwikiserialinit();
}
int is_transmit_empty() { return inb(0x3F8 + 5) & 0x20; }
static char buffer[128];
static size_t idx = 0;
void tputc(int ch, void *ctx) {

  if (ft_ctx == NULL) {
    char c = ch;
    while (is_transmit_empty() == 0)
      ;
    outb(0x3F8, (uint8_t)c);
    return;
  }
  char c = ch;
  if (idx == sizeof(buffer)) {
    flanterm_write(ctx, buffer, idx);
    idx = 0;
  }
  buffer[idx] = c;
  idx++;
  if (c == '\n') {
    flanterm_write(ft_ctx, buffer, idx);
    idx = 0;
  }

#if defined(__x86_64__)
  // THIS IS SO FUCKING HACKY SKULL EMOJI
  while (is_transmit_empty() == 0)
    ;
  outb(0x3F8, (uint8_t)c);
#endif
}
struct flanterm_context *get_fctx() { return ft_ctx; }
void kprintf(const char *format, ...) {
  uint64_t flags;
  asm volatile("pushfq; cli; pop %0" : "=r"(flags));
  spinlock_lock(&lock);
  va_list args;
  va_start(args, format);

  npf_vpprintf(tputc, NULL, format, args);
  if (idx != 0) {
    flanterm_write(ft_ctx, buffer, idx);
    idx = 0;
  }
  va_end(args);
  spinlock_unlock(&lock);
  if (flags & 1 << 9) {
    asm volatile("sti");
  }
}

void sputc(int ch, void *) {
#if defined(__x86_64__)
  char c = ch;
  while (is_transmit_empty() == 0)
    ;
  outb(0x3F8, (uint8_t)c);
#endif
}
void sprintf_write(char *buf, size_t size) {
  for (int i = 0; i < size; i++) {
    sputc(buf[i], NULL);
  }
}
void sprintf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  npf_vpprintf(sputc, NULL, format, args);
  va_end(args);
}
void stolen_osdevwikiserialinit() {
#if defined(__x86_64__)
  outb(0x3F8 + 1, 0x00); // Disable all interrupts
  outb(0x3F8 + 3, 0x80); // Enable DLAB (set baud rate divisor)
  outb(0x3F8 + 0, 0x0C); // Set divisor to 3 (lo byte) 9600 baud
  outb(0x3F8 + 1, 0x00); //                  (hi byte)
  outb(0x3F8 + 3, 0x03); // 8 bits, no parity, one stop bit
  outb(0x3F8 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
  outb(0x3F8 + 4, 0x0B); // IRQs enabled, RTS/DSR set
  outb(0x3F8 + 4, 0x1E); // Set in loopback mode, test the serial chip
  outb(0x3F8 + 0, 0xAE); // Test serial chip (send byte 0xAE and check if serial
                         // returns same byte)
  outb(0x3F8 + 4, 0x0F);
#endif
}
