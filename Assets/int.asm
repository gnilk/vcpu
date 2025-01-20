
    .code
    .org 0x0000

struct isrTable {
    reserved_sp    rs.l    1
    reserved_ip    rs.l    1
    exceptions     rs.l    8
    isr0           rs.l    1
    isr1           rs.l    1
    isr2           rs.l    1
    isr3           rs.l    1
    isr4           rs.l    1
    isr5           rs.l    1
    isr6           rs.l    1
    isr7           rs.l    1
}

isrTable:
    dc.struct   isrTable {}
    .org 0x1000
isrTimer:
    ;move.l  (a0+isrTable.isr0),d0


    add.l   counter,1
    lea     a0, str_hello
    move.l  d0, 0x01
    syscall
    rti

    .org 0x2000
_start:
    lea     a0, str_isr
    move.l  d0, 0x01
    syscall

    ; setup ISR entry
    lea     a0, isrTable
    lea     d0, isrTimer
    move.l  (a0+isrTable.isr0),d0
    ; move.l  (a0+8<<6),d0
    ; enable interrupt
    ; move.l  d0, 0x01
    ; move.l  cr0,d0
    ; wait for counter to become 3
wait_lp:
    add.l   counter, 1
    cmp.l   counter, 3
    bne     wait_lp
; print we are done
    lea     a0, str_done
    move.l  d0, 0x01
    syscall
    ret

    .data
counter:
    dc.l    0
str_hello:
    dc.b    "hello world",0      ;; you need to terminate the strings!!!
str_done:
    dc.b    "done",0      ;; you need to terminate the strings!!!

str_isr:
    dc.b    "setup ISR and enable INT0",0      ;; you need to terminate the strings!!!
