    .code
    .org 0x2000
    add.l counter,1
    move.l d0, counter
    ret
    brk
    .data
counter:    dc.l    0

