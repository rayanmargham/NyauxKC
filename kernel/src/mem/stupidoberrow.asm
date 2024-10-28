global switch_to_pagemap
global invlpg
invlpg:
    invlpg [rdi]
    ret
switch_to_pagemap:
    mov cr3, rdi
    ret