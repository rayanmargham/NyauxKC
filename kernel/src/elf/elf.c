#include "elf.h"
#include "limine.h"
#include <stdint.h>

Elf64_Ehdr *get_kernel_elfheader() {
  return (Elf64_Ehdr *)kernelfile.response->kernel_file->address;
}
uint64_t get_kerneL_address() {
  return (uint64_t)kernelfile.response->kernel_file->address;
}
