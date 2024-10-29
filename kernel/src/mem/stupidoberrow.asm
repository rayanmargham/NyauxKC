global switch_to_pagemap
global invlpg
global outw
global outd
global outb
global inb 
global inw
global ind
invlpg:
    invlpg [rdi]
    ret
switch_to_pagemap:
    mov cr3, rdi
    ret
outb:
	mov dx, di
	mov al, sil
	out dx, al
	ret
outw:
	mov dx, di
	mov ax, si
	out dx, ax
	ret
outd:
	mov dx, di
	mov eax, esi
	out dx, eax
	ret
inb:
	mov eax, 0
	mov dx, di
	in al, dx
	ret
inw:
	mov eax, 0
	mov dx, di
	in ax, dx
	ret
ind:
	mov dx, di
	in eax, dx
	ret