;----------------------
; asm file
;----------------------
    .org 0x100
    .code
; BASE EQU 45


    lea     a0, datablock
    move.b  d0, (a0)
    brk

    .data
datablock:
    dc.b    0xff,2,3,4,5,6,7

    .text