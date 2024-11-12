#include "elf/symbols/symbols.h"
#include "gdt/gdt.h"
#include "idt/idt.h"
#include "mem/vmm.h"
#include "term/term.h"
#include "utils/basic.h"
#include <acpi/acpi.h>
#include <elf/symbols/symbols.h>
#include <limine.h>
#include <mem/kmem.h>
#include <mem/pmm.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Set the base revision to 2, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used,
               section(".requests"))) static volatile LIMINE_BASE_REVISION(2)

    // The Limine requests can be placed anywhere, but it is important that
    // the compiler does not optimise them away, so, usually, they should
    // be made volatile or equivalent, _and_ they should be accessed at least
    // once or marked as used with the "used" attribute as done here.

    __attribute__((
        used,
        section(".requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 2};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used,
               section(".requests_start_"
                       "marker"))) static volatile LIMINE_REQUESTS_START_MARKER
    __attribute__((used,
                   section(".requests"))) volatile struct limine_memmap_request
        memmap_request = {.id = LIMINE_MEMMAP_REQUEST, .revision = 2};
__attribute__((used, section(".requests"))) volatile struct limine_hhdm_request
    hhdm_request = {.id = LIMINE_HHDM_REQUEST, .revision = 2};
__attribute__((
    used, section(".requests"))) volatile struct limine_kernel_address_request
    kernel_address = {.id = LIMINE_KERNEL_ADDRESS_REQUEST, .revision = 2};
__attribute__((used, section(".requests"))) volatile struct limine_rsdp_request
    rsdp_request = {.id = LIMINE_RSDP_REQUEST, .revision = 2};
__attribute__((used,
               section(".requests"))) volatile struct limine_kernel_file_request
    kernelfile = {.id = LIMINE_KERNEL_FILE_REQUEST, .revision = 2};
__attribute__((
    used,
    section(".requests_end_marker"))) static volatile LIMINE_REQUESTS_END_MARKER

    // GCC and Clang reserve the right to generate calls to the following
    // 4 functions even if they are not directly called.
    // Implement them as the C specification mandates.
    // DO NOT remove or rename these functions, or stuff will eventually break!
    // They CAN be moved to a different .c file.

    void *
    memcpy(void *dest, const void *src, size_t n) {
  uint8_t *pdest = (uint8_t *)dest;
  const uint8_t *psrc = (const uint8_t *)src;

  for (size_t i = 0; i < n; i++) {
    pdest[i] = psrc[i];
  }

  return dest;
}

void *memset(void *s, int c, size_t n) {
  uint8_t *p = (uint8_t *)s;

  for (size_t i = 0; i < n; i++) {
    p[i] = (uint8_t)c;
  }

  return s;
}

void *memmove(void *dest, const void *src, size_t n) {
  uint8_t *pdest = (uint8_t *)dest;
  const uint8_t *psrc = (const uint8_t *)src;

  if (src > dest) {
    for (size_t i = 0; i < n; i++) {
      pdest[i] = psrc[i];
    }
  } else if (src < dest) {
    for (size_t i = n; i > 0; i--) {
      pdest[i - 1] = psrc[i - 1];
    }
  }

  return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const uint8_t *p1 = (const uint8_t *)s1;
  const uint8_t *p2 = (const uint8_t *)s2;

  for (size_t i = 0; i < n; i++) {
    if (p1[i] != p2[i]) {
      return p1[i] < p2[i] ? -1 : 1;
    }
  }

  return 0;
}

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
void kmain(void) {
  // Ensure the bootloader actually understands our base revision (see spec).
  if (LIMINE_BASE_REVISION_SUPPORTED == false) {
    hcf();
  }

  // Ensure we got a framebuffer.
  if (framebuffer_request.response == NULL ||
      framebuffer_request.response->framebuffer_count < 1) {
    hcf();
  }

  // Fetch the first framebuffer.
  struct limine_framebuffer *framebuffer =
      framebuffer_request.response->framebuffers[0];
  init_term(framebuffer);
  kprintf("Hello World ITS THE BEST NYAUX REWRITE EVER HERE\nDEF NOT FAMOUS "
          "LAST WORDS HERES FUNNY NUMBER TO SHOW WE USING NANOPRINTF %d\n",
          69420);
  init_gdt();
  kprintf("GDT Inited.\n");

  // We're done, just hang...
  init_idt();
  kprintf("YOOO LETS THIS THIS OUT\n");
  assert(1 == 1);
  result r = pmm_init();
  unwrap_or_panic(r);
  vmm_init();
  init_acpi();
  free_unused_slabcaches();
  kprintf("Total Memory in Use: %u Bytes or %u MB\n", total_memory(),
          total_memory() / 1048576);

  get_symbols();

  panic("Uhhh yeah wssup");
}
