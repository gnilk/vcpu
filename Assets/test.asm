

    lea     a0, datablock
    move.b  d0, (a0)
    brk

datablock:
    dc.b    1,2,3,4,5,6,7