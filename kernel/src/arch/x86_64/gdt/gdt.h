#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <term/term.h>
void init_gdt();
struct TSS {
  uint32_t reversed;
  uint64_t rsp[3]; // rsp 0, rsp 1, rsp 2
  uint64_t NOTALLOWED;
  uint64_t ists[7];
  uint64_t NONO;
  uint16_t NONONO;
  uint16_t IOPB;
} __attribute__((packed));
void change_rsp0(uint64_t stackaddr);
#ifdef __cplusplus
}
#endif