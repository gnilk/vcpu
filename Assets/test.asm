;----------------------
; asm file
;----------------------
    .org 0x100
    .code
; BASE EQU 45


    lea     a0, datablock
    move.b  d0, (a0)
    brk

;
; this will be merged at the end anyway, if you want data items within your code-statements you should not switch
; segments... just declare them up front..
;
    .data
datablock:
    dc.b    0xff,2,3,4,5,6,7

    .text