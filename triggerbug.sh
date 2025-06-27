#!/bin/sh
./jinx rebuild kernel
make
qemu-system-x86_64 -qmp unix:/tmp/qmp.sock,server,nowait -M q35 -m 1G --enable-kvm -cpu host -cdrom nyaux.iso -serial stdio &
notify-send "type in alias hi='cat /dev/keyboard'"
sleep 15
./ok.py /tmp/qmp.sock "hi\n" 0.01
