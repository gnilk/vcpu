
; This should mimic the IST_VECTOR_TABLE struct from Interrupt.h
struct ISRVectorTable {
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
    reserved       rs.l    16
}
struct ExceptionControlBlock {
    flags            rs.l    1
    registers        rs.b sizeof(ISRVectorTable)

}

    .code
    .org 0x0000
isrTable:
    dc.struct   ISRVectorTable {}
