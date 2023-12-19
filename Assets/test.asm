;----------------------
; asm file
;----------------------
; struct int_table absolute 0     ;; do I want this??   I don't need it - sugar...
;    syscall_1   rs.l    1       ;; rs = ReServe
;    syscall_2   rs.l    1
;    syscall_3   rs.l    1
; endstruct

    .org 0x2000
    .code
; BASE EQU 45
;    move.l  a0, 0       ;; address of int.table
;    lea     a0, int_table           ;; should load the absolute address of 0
;    call    int_table.syscall_1      ;; this would work, if I can support 'absolute' need to see how the compiler handles this...

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