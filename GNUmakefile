# GNUmakefile: Main makefile of the project.
# Code is governed by the GPL-2.0 license.
# Copyright (C) 2021-2024 The nyaux authors.

QEMUFLAGS ?= -M q35,smm=off -m 2G -cdrom nyaux.iso -serial stdio -smp 4

.PHONY: all
all:
	rm -f nyaux.iso
	$(MAKE) nyaux.iso

nyaux.iso: jinx
	./build-support/makeiso.sh

.PHONY: debug
debug:
	JINX_CONFIG_FILE=jinx-config-debug $(MAKE) all

jinx:
	curl -Lo jinx https://github.com/mintsuki/jinx/raw/7a101a39eb061713f9c50ceafa1d713f35f17a3b/jinx
	chmod +x jinx

.PHONY: run-kvm
run-kvm: nyaux.iso
	qemu-system-x86_64 -enable-kvm -cpu host $(QEMUFLAGS)

.PHONY: run-hvf
run-hvf: nyaux.iso
	qemu-system-x86_64 -accel hvf -cpu host $(QEMUFLAGS)

ovmf/ovmf-code-x86_64.fd:
	mkdir -p ovmf
	curl -Lo $@ https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/ovmf-code-x86_64.fd

ovmf/ovmf-vars-x86_64.fd:
	mkdir -p ovmf
	curl -Lo $@ https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/ovmf-vars-x86_64.fd

.PHONY: run-uefi
run-uefi: nyaux.iso ovmf/ovmf-code-x86_64.fd ovmf/ovmf-vars-x86_64.fd
	qemu-system-x86_64 \
		-enable-kvm \
		-cpu host \
		-drive if=pflash,unit=0,format=raw,file=ovmf/ovmf-code-x86_64.fd,readonly=on \
		-drive if=pflash,unit=1,format=raw,file=ovmf/ovmf-vars-x86_64.fd \
		$(QEMUFLAGS)

.PHONY: run-bochs
run-bochs: nyaux.iso
	bochs -f bochsrc

.PHONY: run-lingemu
run-lingemu: nyaux.iso
	lingemu runvirt -m 8192 --diskcontroller type=ahci,name=ahcibus1 --disk nyaux.iso,disktype=cdrom,controller=ahcibus1

.PHONY: run
run: nyaux.iso
	qemu-system-x86_64 $(QEMUFLAGS)

.PHONY: clean
clean:
	rm -rf iso_root sysroot nyaux.iso initramfs.tar

.PHONY: distclean
distclean: clean
	make -C kernel distclean
	rm -rf .jinx-cache jinx builds host-builds host-pkgs pkgs sources ovmf