    .code
    .org 0x2000

_start:
        add.l   counter,1
        cmp.l   counter,3
        bne     _start
        ret
        brk
    .data
counter:    dc.l    0
