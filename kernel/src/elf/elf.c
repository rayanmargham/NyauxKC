#include "elf.h"
#include "limine.h"

Elf64_Ehdr *get_kernel_elfheader() {
  return (Elf64_Ehdr *)kernelfile.response->kernel_file->address;
}
