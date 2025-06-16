supported_archs = ['x86_64']
if not supported_archs.contains(host_machine.cpu_family())
  error('Nyaux does not support this arch')
endif
src += files(
  'arch.c',
)
if host_machine.cpu_family() == 'x86_64'
  src += files(

	'x86_64/cmos/cmos.c',
	'x86_64/cpu/lapic.c',
	'x86_64/fpu/xsave.c',
	'x86_64/gdt/gdt.c',
	'x86_64/idt/idt.c',
	'x86_64/interrupt_controllers/ioapic.c',
	'x86_64/page_tables/pt.c',
	'x86_64/syscalls/syscall.c',
	'x86_64/gdt/gdte.asm',
	'x86_64/idt/idtstub.asm',
	'x86_64/page_tables/stupidoberrow.asm',
	'x86_64/syscalls/syscalls.asm',
  )
endif
