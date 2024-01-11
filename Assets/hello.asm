//
// Created by gnilk on 11.01.2024.
//
    .code

    .data
_start:
    lea     a0, string
    move.l  d0, 0x01
    syscall
    ret

string:
    dc.b    "hello world",0      ;; you need to terminate the strings!!!
