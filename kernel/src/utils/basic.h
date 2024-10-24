#pragma once
static void hcf(void) {
  for (;;) {
#if defined(__x86_64__)
    asm("hlt");
#elif defined(__aarch64__) || defined(__riscv)
    asm("wfi");
#elif defined(__loongarch64)
    asm("idle 0");
#endif
  }
}