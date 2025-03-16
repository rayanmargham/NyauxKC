#include <limine.h>
#include <stdint.h>
#define EI_NIDENT 16
#define Elf64_Addr uint64_t
#define Elf64_Off uint64_t
#define Elf64_Half uint16_t
#define Elf64_Word uint32_t
#define Elf64_Sword int
#define Elf64_Xword uint64_t
#define Elf64_Sxword signed long int
typedef struct {
  unsigned char e_ident[EI_NIDENT];
  Elf64_Half e_type;
  Elf64_Half e_machine;
  Elf64_Word e_version;
  Elf64_Addr e_entry;
  Elf64_Off e_phoff;
  Elf64_Off e_shoff;
  Elf64_Word e_flags;
  Elf64_Half e_ehsize;
  Elf64_Half e_phentsize;
  Elf64_Half e_phnum;
  Elf64_Half e_shentsize;
  Elf64_Half e_shnum;
  Elf64_Half e_shstrndx;
} Elf64_Ehdr;
typedef struct {
  Elf64_Word sh_name;       /* Section name */
  Elf64_Word sh_type;       /* Section type */
  Elf64_Xword sh_flags;     /* Section attributes */
  Elf64_Addr sh_addr;       /* Virtual address in memory */
  Elf64_Off sh_offset;      /* Offset in file */
  Elf64_Xword sh_size;      /* Size of section */
  Elf64_Word sh_link;       /* Link to other section */
  Elf64_Word sh_info;       /* Miscellaneous information */
  Elf64_Xword sh_addralign; /* Address alignment boundary */
  Elf64_Xword sh_entsize;   /* Size of entries, if section has table */
} Elf64_Shdr;
typedef struct {
  Elf64_Word st_name;
  unsigned char st_info;
  unsigned char st_other;
  Elf64_Half st_shndx;
  Elf64_Addr st_value;
  Elf64_Xword st_size;
} Elf64_Sym;
typedef struct {
  Elf64_Word p_type;
  Elf64_Word p_flags;
  Elf64_Off p_offset;
  Elf64_Addr p_vaddr;
  Elf64_Addr p_paddr;
  Elf64_Xword p_filesz;
  Elf64_Xword p_memsz;
  Elf64_Xword p_align;
} Elf64_Phdr;
typedef struct {
  Elf64_Sxword d_tag;
  union {
    Elf64_Xword d_val;
    Elf64_Addr d_ptr;
  } d_un;
} Elf64_Dyn;
extern volatile struct limine_executable_file_request kernelfile;
Elf64_Ehdr *get_kernel_elfheader();
uint64_t get_kerneL_address();
void test_elf();