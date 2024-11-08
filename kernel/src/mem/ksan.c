#include "ksan.h"
#include <stddef.h>
#include <stdint.h>

// copied from OBOS
const uint8_t nyaux_posion[] = {
    0xFD, // alocated
    0xAA, // free
    0x1A, // non zeroed mem
};
bool memcmp_b(void *addr, uint64_t size, uint8_t val) {
  const uint8_t *p1 = (const uint8_t *)addr;
  for (int i = 0; i < size; i++) {
    if (p1[i] != val) {
      return false;
    }
  }
  return true;
}
#define round_down_to_page(addr)                                               \
  ((uintptr_t)(addr) - ((uintptr_t)(addr) % 4096))
#define NYAUX_CROSSES_PAGE_BOUNDARY(base, size)                                \
  (round_down_to_page(base) == round_down_to_page((uintptr_t)(base) + (size)))
NYAUX_NO_KSAN static void asan_shadow_space_access(uint64_t at, uint64_t size,
                                                   uint64_t ip, bool rw,
                                                   uint8_t posionindex,
                                                   bool abort) {
  if (NYAUX_CROSSES_PAGE_BOUNDARY(at - 16, 16) ||
      NYAUX_CROSSES_PAGE_BOUNDARY(at + size, 16)) {
    return;
  }
  bool isposleft =
      memcmp_b(((uint64_t *)(uint64_t)at - 16), 16, nyaux_posion[posionindex]);
  bool isposright = memcmp_b(((uint64_t *)(uint64_t)at + size), 16,
                             nyaux_posion[posionindex]);
  if (isposleft || isposright) {
    switch (posionindex) {
    case 0xFD:
      kprintf("NyauxSan: Violation of allocated region when trying to use %p "
              "to %s %lu bytes, at %p",
              at, rw ? "write" : "read", size, ip);
      panic("NyauxSan Violation");
      break;
    default:
      panic("NyauxSan: Unknown Violation (i didnt impl it yet lol)");
      break;
    }
  }
}
NYAUX_NO_KSAN static void asan_verify(uint64_t at, uint64_t size, uint64_t ip,
                                      bool rw, bool abort) {
  if (!((uint64_t)(at >> 47) == 0 || ((uint64_t)at >> 47) == 0x1ffff)) {
    kprintf("NyauxSan: Invalid Address access happened at %p, address is %p "
            "trying to %s",
            ip, at, rw ? "write" : "read");
    panic("NyauxSan Violation");
  }
  if (memcmp_b((uint64_t *)at, size, nyaux_posion[0])) {
    asan_shadow_space_access(at, size, ip, rw, 0, abort);
  }
  if (memcmp_b((uint64_t *)at, size, nyaux_posion[1])) {
    asan_shadow_space_access(at, size, ip, rw, 1, abort);
  }
  if (memcmp_b((uint64_t *)at, size, nyaux_posion[2])) {
    asan_shadow_space_access(at, size, ip, rw, 2, abort);
  }
}
NYAUX_NO_KSAN void __asan_load1_noabort(uint64_t addr) {
  asan_verify(
      addr, 1,
      (uint64_t)__builtin_extract_return_addr(__builtin_return_address(0)),
      false, true);
}
NYAUX_NO_KSAN void __asan_load2_noabort(uint64_t addr) {
  asan_verify(
      addr, 2,
      (uint64_t)__builtin_extract_return_addr(__builtin_return_address(0)),
      false, true);
}
NYAUX_NO_KSAN void __asan_load4_noabort(uint64_t addr) {
  asan_verify(
      addr, 4,
      (uint64_t)__builtin_extract_return_addr(__builtin_return_address(0)),
      false, true);
}
NYAUX_NO_KSAN void __asan_load8_noabort(uint64_t addr) {
  asan_verify(
      addr, 8,
      (uint64_t)__builtin_extract_return_addr(__builtin_return_address(0)),
      false, true);
}
NYAUX_NO_KSAN void __asan_load16_noabort(uint64_t addr) {
  asan_verify(
      addr, 16,
      (uint64_t)__builtin_extract_return_addr(__builtin_return_address(0)),
      false, true);
}
NYAUX_NO_KSAN void __asan_store1_noabort(uint64_t addr) {
  asan_verify(
      addr, 1,
      (uint64_t)__builtin_extract_return_addr(__builtin_return_address(0)),
      true, true);
}
NYAUX_NO_KSAN void __asan_store2_noabort(uint64_t addr) {
  asan_verify(
      addr, 2,
      (uint64_t)__builtin_extract_return_addr(__builtin_return_address(0)),
      true, true);
}
NYAUX_NO_KSAN void __asan_store4_noabort(uint64_t addr) {
  asan_verify(
      addr, 4,
      (uint64_t)__builtin_extract_return_addr(__builtin_return_address(0)),
      true, true);
}
NYAUX_NO_KSAN void __asan_store8_noabort(uint64_t addr) {
  asan_verify(
      addr, 8,
      (uint64_t)__builtin_extract_return_addr(__builtin_return_address(0)),
      true, true);
}
NYAUX_NO_KSAN void __asan_store16_noabort(uint64_t addr) {
  asan_verify(
      addr, 16,
      (uint64_t)__builtin_extract_return_addr(__builtin_return_address(0)),
      true, true);
}
NYAUX_NO_KSAN void __asan_storeN_noabort(uint64_t addr, size_t size) {
  asan_verify(
      addr, (uint64_t)size,
      (uint64_t)__builtin_extract_return_addr(__builtin_return_address(0)),
      true, true);
}
NYAUX_NO_KSAN void __asan_loadN_noabort(uint64_t addr, size_t size) {
  asan_verify(
      addr, (uint64_t)size,
      (uint64_t)__builtin_extract_return_addr(__builtin_return_address(0)),
      false, true);
}
NYAUX_NO_KSAN void __asan_handle_no_return() { return; }
