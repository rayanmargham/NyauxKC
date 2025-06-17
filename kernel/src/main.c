#include <arch/x86_64/cmos/cmos.h>
#include <dbg/kdb.h>
#include <elf/symbols/symbols.h>
#include <flanterm/src/flanterm.h>
#include <fs/vfs/fd.h>
#include <utils/cmdline.hpp>
#include <fs/vfs/vfs.h>
#include <mem/vmm.h>
#include <sched/sched.h>
#include <smp/smp.h>
#include <term/term.h>
#include <uacpi/status.h>
#include <utils/basic.h>
#include <utils/libc.h>
#include <Mutexes/KMutex.h>
#include <acpi/acpi.h>
#include <arch/arch.h>
#include <elf/symbols/symbols.h>
#include <fs/vfs/vfs.h>
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
        used, section(".requests"))) volatile struct limine_framebuffer_request
    framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 3};
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
    smp_request = {.id = LIMINE_MP_REQUEST, .revision = 3};
__attribute__((
    used,
    section(".requests"))) volatile struct limine_executable_address_request
    kernel_address = {.id = LIMINE_EXECUTABLE_ADDRESS_REQUEST, .revision = 3};
__attribute__((used, section(".requests"))) volatile struct limine_rsdp_request
    rsdp_request = {.id = LIMINE_RSDP_REQUEST, .revision = 3};
__attribute__((
    used, section(".requests"))) volatile struct limine_executable_file_request
    kernelfile = {.id = LIMINE_EXECUTABLE_FILE_REQUEST, .revision = 3};
__attribute__((
    used,
    section(".requests"))) volatile struct limine_module_request modules = {
    .id = LIMINE_MODULE_REQUEST, .revision = 3};
__attribute__((
    used,
    section(".requests"))) volatile struct limine_boot_time_request limine_boot_time = {
    .id = LIMINE_BOOT_TIME_REQUEST, .revision = 3};
__attribute__((used, section(".requests"))) volatile struct limine_executable_cmdline_request limine_cmdline = {
    .id = LIMINE_EXECUTABLE_CMDLINE_REQUEST, .revision = 3
};
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
                       /* No outputs */
                       : "+D"(s), // RDI -> destination pointer

                         "+c"(n) // RCX -> number of quadwords to fill
                       : "a"(c)  // RAX -> value to store
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
#include <uacpi/sleep.h>
// Nyaux Kernel Entry Point
// extern void nofriendsnobitches();
void kmain(void) {
  // Ensure the bootloader actually understands our base revision (see spec).
  if (LIMINE_BASE_REVISION_SUPPORTED == false) {
    hcf();
  }
  // nofriendsnobitches();
  // Ensure we got a framebuffer.
  if (framebuffer_request.response == NULL ||
      framebuffer_request.response->framebuffer_count < 1) {
      sprintf_log(FATAL, "Cannot Continue without FrameBuffer\r\n");
      hcf();
  }
  // Fetch the first framebuffer.
  struct limine_framebuffer *framebuffer =
      framebuffer_request.response->framebuffers[0];
  init_term(framebuffer);
  arch_init();
  // We're done, just hang...
  result r = pmm_init();
  unwrap_or_panic(r);
  vmm_init();
  reinit_term(framebuffer);
  parse_cmdline();
  get_symbols();
  init_acpi_early();
  kprintf_log(TRACE, "kmain(): Total Memory in Use: %lu Bytes or %lu MB\r\n",
          total_memory(), total_memory() / 1048576);
  create_kentry();
  hashmap_set_allocator(kmalloc, kfree);
  init_smp();
  // uacpi_status ret = uacpi_prepare_for_sleep_state(UACPI_SLEEP_STATE_S5);
  // if (uacpi_unlikely_error(ret))
  // {
  // 	kprintf("Failed to shutdown system. %s\n", uacpi_status_to_string(ret));
  // }
  // __asm__ volatile("cli");
  // ret = uacpi_enter_sleep_state(UACPI_SLEEP_STATE_S5);
  // if (uacpi_unlikely_error(ret))
  // {
  // 	kprintf("Failed to shutdown system. %s\n", uacpi_status_to_string(ret));
  // }
  kprintf_log(TRACE, "kmain(): We are chilling i guess\n");
  hcf(); // We just chill.
}
extern void do_funny();
void kentry() {
  // init vfs, load font from initramfs, change font or smthin for flanterm or
  // use a custom made terminal idk, lots of things to do
  kprintf("kentry(): Hello World from a scheduled thread\r\n");
  vfs_init();
  get_time();
  // struct ring_buf *funnytest = init_ringbuf(10);
  // while (put_ringbuf(funnytest, 5))
  //   ;
  // ;
  // uint64_t ret = 0;
  // while (get_ringbuf(funnytest, (uint64_t *)&ret)) {
  //   kprintf("ringbuf(): %d\r\n", (int)ret);
  // }
  // pagemap *test = new_pagemap();
  // kprintf("%p\r\n", test);
  // duplicate_pagemap(&ker_map, test);
  // kprintf("what \r\n");
  // arch_switch_pagemap(test);
  // kprintf("now in different pagemap\r\n");
  // kprintf("yay\n");
  // struct process_t *proc = get_process_start();
  // proc->cur_map = test;
  // get_process_finish(proc);
  do_funny();
  // rsh();
  exit_thread();
}
