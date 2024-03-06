    .code
    .org 0x2000

_start:
        lea     a0, str_start
        move.l  d0, 0x01
        syscall

        move.l  counter,0
wait_lp:
        add.l   counter, 1
        cmp.l   counter, 3
        bne     wait_lp

        lea     a0, str_done
        move.l  d0, 0x01
        syscall
        ret
        brk

    .data
counter:    dc.l    0
str_start:  dc.b    "Starting app",0
str_done:   dc.b    "Done with loop",0