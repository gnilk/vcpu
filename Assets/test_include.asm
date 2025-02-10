

    .include "Assets/src_to_include.inc"
    .include "sys/isrtable.asm"
;    # I used this to test include path settings, but they don't propagate yet when making so commenting it out
;    # .include "sys/dummy.inc"

    .code
    .org 0x2000
_start:
; this should work
    move.l d0, A_CONSTANT
;   # move.l d1, B_CONSTANT
    ret
