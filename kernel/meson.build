project(
  'NyauxKC',
  'c',
  'cpp',
  'nasm',
  default_options: ['c_std=gnu23', 'cpp_std=gnu++23', 'warning_level=2'],
)
target_arch = host_machine.cpu_family()
uacpi = subproject('uacpi')
frigg = dependency('frigg')
cc_runtime = dependency('cc_runtime')
freestnd_c_hdrs = dependency('freestnd_c_hdrs')
freestnd_cxx_hdrs = dependency('freestnd_cxx_hdrs')
src = files(
  'src/sched/test.asm',
  'src/sched/yield.asm',
	'src/Mutexes/KMutex.c',
	'src/Mutexes/seqlock.c',
	'src/acpi/acpi.c',
	'src/acpi/ec.c',
	'src/acpi/kernelapi.c',
	'src/cppglue/glue.cpp',
	'src/dbg/kdb.c',
	'src/dev/fbdev/fb.cpp',
	'src/dev/null/null.c',
	'src/dev/tty/tty.c',
	'src/elf/elf.c',
	'src/elf/symbols/symbols.c',
	'src/flanterm/src/flanterm_backends/fb.c',
	'src/flanterm/src/flanterm.c',
	'src/fs/devfs/devfs.c',
	'src/fs/tmpfs/tmpfs.c',
	'src/fs/ustar/ustar.c',
	'src/fs/vfs/fd.c',
	'src/fs/vfs/vfs.c',
	'src/main.c',
	'src/mem/kmem.c',
	'src/mem/pmm.c',
	'src/mem/sanitizers/UBsan/ubsan.c',
	'src/mem/sanitizers/ksan/ksan.c',
	'src/mem/vmm.c',
	'src/pci/pci.c',
	'src/sched/reaper.c',
	'src/sched/sched.c',
	'src/smp/smp.c',
	'src/term/term.c',
	'src/timers/hpet.cpp',
	'src/timers/pvclock.cpp',
	'src/timers/timer.cpp',
	'src/utils/cmdline.cpp',
	'src/utils/hashmap.c',
	'src/utils/libc.c',
	'src/utils/profiler.c',
    'src/controllers/i8042/i8042.c',
    'src/dev/keyboard/keyboard.cpp'
)

inc = include_directories(
	'src/',
)
link_args = [
    '-nostdlib',
    '-static',
    '-Wl,--build-id=none',
    '-Wl,--gc-sections',
    '-Wl,-z,max-page-size=0x1000',
    '-Wl,-T,' + meson.current_source_dir() / f'linker-@target_arch@.ld',
]
c_args = [

]
cpp_args = [
	'-fno-rtti',
	'-fno-exceptions',
]
add_project_arguments(
    '-g',
    '-nostdinc',
    '-ffreestanding',
    '-fno-stack-protector',
    '-fno-stack-check',
    '-ffunction-sections',
    '-fdata-sections',
    '-DUACPI_SIZED_FREES',
    '-DLIMINE_API_REVISION=2',
    language: ['c', 'cpp'],
)

subdir('src/arch')
shared_args = ['-fno-PIC']
if target_arch == 'x86_64'
    shared_args += [
        '-m64',
        '-march=x86-64',
        '-mno-80387',
        '-mno-mmx',
        '-mno-sse',
        '-mno-sse2',
        '-mno-red-zone',
        '-mcmodel=kernel',
    ]

    link_args += ['-Wl,-m,elf_x86_64']
elif target_arch == 'aarch64'
    shared_args += ['-mgeneral-regs-only']

    link_args += ['-Wl,-m,aarch64elf']
elif target_arch == 'riscv64'
    shared_args += [
        '-march=rv64imac_zicsr_zifencei',
        '-mabi=lp64',
        '-mno-relax',
    ]

    link_args += [
        '-Wl,-m,elf64lriscv',
        '-Wl,--no-relax',
    ]
elif target_arch == 'loongarch64'
    shared_args += [
        '-march=loongarch64',
        '-mabi=lp64s',
    ]

    link_args += [
        '-Wl,-m,elf64loongarch',
        '-Wl,--no-relax',
    ]
else
    error('Unsupported architecture: ' + target_arch)
endif
executable('NyauxKC', src, uacpi.get_variable('sources'),
	dependencies: [frigg, freestnd_c_hdrs, freestnd_cxx_hdrs, cc_runtime],
	c_args: c_args + shared_args,
	cpp_args: cpp_args + shared_args,
	link_args: link_args,
	include_directories: [inc, uacpi.get_variable('includes')],
	install: true
)
