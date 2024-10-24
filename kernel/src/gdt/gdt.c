#include "gdt.h"

typedef struct {
  uint16_t size;
  uint64_t offset;
} __attribute__((packed)) GDTR;

uint64_t gdt[9];
extern void gdt_flush(void *);
GDTR gptr = {};
void init_gdt() {
  gdt[0] = 0x0;
  gdt[1] = 0x00009a000000ffff;
  gdt[2] = 0x000093000000ffff;
  gdt[3] = 0x00cf9a000000ffff;
  gdt[4] = 0x00cf93000000ffff;
  gdt[5] = 0x00af9b000000ffff;
  gdt[6] = 0x00af93000000ffff;
  gdt[7] = 0x00aff3000000ffff;
  gdt[8] = 0x00affb000000ffff;
  gptr.offset = (uint64_t)&gdt;
  gptr.size = sizeof(gdt);
  kprintf("Offset is %lx\n", gptr.offset);
  kprintf("Size is %d\n", gptr.size);

  gdt_flush(&gptr);
}