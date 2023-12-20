;----------------------
; asm file
;----------------------
; struct int_table absolute 0     ;; do I want this??   I don't need it - sugar...
;    syscall_1   rs.l    1       ;; rs = ReServe
;    syscall_2   rs.l    1
;    syscall_3   rs.l    1
; endstruct

    ; .org 0x2000
    .code
; BASE EQU 45
    lea     a0, funcA
    call    a0          ;; call the value 'a0' points at, assume you have jump-table you could do call (a0)

    lea     a0, datablock
    move.b  d0, (a0)
    brk
funcA:
    move.l  d0, 0x01
    syscall
    ret

;
; this will be merged at the end anyway, if you want data items within your code-statements you should not switch
; segments... just declare them up front..
;
    .data
datablock:
    dc.b    0xff,2,3,4,5,6,7

    .text