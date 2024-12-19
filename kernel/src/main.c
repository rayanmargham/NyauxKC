#include "elf/symbols/symbols.h"
#include "mem/vmm.h"
#include "smp/smp.h"
#include "term/term.h"
#include "utils/basic.h"
#include <acpi/acpi.h>
#include <arch/arch.h>
#include <elf/symbols/symbols.h>
#include <limine.h>
#include <mem/kmem.h>
#include <mem/pmm.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <timers/hpet.h>

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
__attribute__((used, section(".requests"))) volatile struct limine_mp_request
    smp_request = {.id = LIMINE_MP_REQUEST, .revision = 2};
__attribute__((
    used,
    section(".requests"))) volatile struct limine_executable_address_request
    kernel_address = {.id = LIMINE_EXECUTABLE_ADDRESS_REQUEST, .revision = 2};
__attribute__((used, section(".requests"))) volatile struct limine_rsdp_request
    rsdp_request = {.id = LIMINE_RSDP_REQUEST, .revision = 2};
__attribute__((
    used, section(".requests"))) volatile struct limine_executable_file_request
    kernelfile = {.id = LIMINE_EXECUTABLE_FILE_REQUEST, .revision = 2};
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
#ifdef __x86_64__
  void *tmptmp = dest;
  __asm__ __volatile__("rep movsb\n\t"
                       : "+D"(dest), "+S"(src), "+c"(n)
                       :
                       : "memory");
  return tmptmp;
#endif
  uint8_t *pdest = (uint8_t *)dest;
  const uint8_t *psrc = (const uint8_t *)src;

  for (size_t i = 0; i < n; i++) {
    pdest[i] = psrc[i];
  }

  return dest;
}

void *memset(void *s, int c, size_t n) {
#ifdef __x86_64__
  void *tmp = s;
  __asm__ __volatile__("rep stosb\n\t" // Repeat STOSQ for RCX times
                       :               /* No outputs */
                       : "D"(s),       // RDI -> destination pointer
                         "a"(c),       // RAX -> value to store
                         "c"(n)        // RCX -> number of quadwords to fill
                       : "memory");
  return tmp;

#endif
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
  kprintf("kmain(): Hello World ITS THE BEST NYAUX REWRITE EVER HERE\nDEF NOT "
          "FAMOUS "
          "LAST WORDS HERES FUNNY NUMBER TO SHOW WE USING NANOPRINTF %d\n",
          69420);

  arch_init();
  // We're done, just hang...

  result r = pmm_init();
  unwrap_or_panic(r);
  vmm_init();
  get_symbols();
  init_acpi();
  kprintf("kmain(): Total Memory in Use: %lu Bytes or %lu MB\n", total_memory(),
          total_memory() / 1048576);

  // init_smp();
  hcf(); // we js chill
}
