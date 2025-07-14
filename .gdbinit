
set debuginfod enabled on
file builds/kernel/NyauxKC
target remote :1234
set substitute-path ../../../base_dir .
add-symbol-file sysroot/lib64/libreadline.so.8.2 0x41000000
add-symbol-file sysroot/bin/bash 0x101000
add-symbol-file sysroot/lib64/libc.so 0x41600000