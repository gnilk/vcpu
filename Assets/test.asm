;----------------------
; asm file
;----------------------
; struct int_table absolute 0     ;; do I want this??   I don't need it - sugar...
;    syscall_1   rs.l    1       ;; rs = ReServe - asmone syntax, we could also use: 'dc.l  <default>' ?
;    syscall_2   rs.l    1
;    syscall_3   rs.l    1
; endstruct

struct table {
    some_byte    rs.b    1
    some_word    rs.w    1
    some_dword   rs.d    1
}

    ; .org 0x2000
    .code
; BASE EQU 45
    lea     a0, funcA
    call    a0          ;; call the value 'a0' points at, assume you have jump-table you could do call (a0)

    lea     a0, string
    move.b  d0, (a0)
    brk
funcA:
    lea     a0, string
    move.l  d0, 0x01
    syscall
    ret

;
; this will be merged at the end anyway, if you want data items within your code-statements you should not switch
; segments... just declare them up front..
;
    .data
data:
    ;; Generates exactly 7 bytes in binary
    dc.struct table {
        dc.b    1       ;; some_byte
        dc.w    2       ;; some_word
        dc.d    3       ;; some_dword
    }

    ;; Empty - will fill with zero => this should go to BSS
    dc.struct table {
    }

    ;; Overflow - will generate all bytes defined below and issue a warning
    dc.struct table {
        dc.w    1,2,3,4
    }

string:
    dc.b    "mamma is a hamster",0      ;; you need to terminate the strings!!!

    .text