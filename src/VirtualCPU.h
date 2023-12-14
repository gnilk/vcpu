//
// Created by gnilk on 14.12.23.
//

#ifndef VIRTUALCPU_H
#define VIRTUALCPU_H

#include <stdint.h>
#include <stdlib.h>
#include "fmt/format.h"

namespace gnilk {


    class IMemReadWrite {
    public:
        virtual uint8_t  ReadU8(uint64_t address) = 0;
        virtual uint16_t ReadU16(uint64_t address) = 0;
        virtual uint32_t ReadU32(uint64_t address) = 0;
        virtual uint64_t ReadU64(uint64_t address) = 0;

        virtual void WriteU8(uint64_t address, uint8_t value) = 0;
        virtual void WriteU16(uint64_t address, uint16_t value) = 0;
        virtual void WriteU32(uint64_t address, uint32_t value) = 0;
        virtual void WriteU64(uint64_t address, uint64_t value) = 0;
    };


    struct RegisterValue {
        union {
            uint8_t byte;
            uint16_t word;
            uint32_t dword;
            uint64_t longword;
        } data;
        // Make sure this is zero when creating a register..
        RegisterValue() {
            data.longword = 0;
        }
    };

    // will need more..
    struct CPUFlags {
        bool carry;     // over/underflow - flow or carry
        bool zero;      // zero
    };

    struct Registers {
        // 8 General purpose registers
        RegisterValue dataRegisters[8];
        // 7 + Stack - address registers
        RegisterValue addressRegisters[7];
        RegisterValue stackPointer;

        // Instruction pointer can't be modified
        RegisterValue instrPointer;
    };

    //
    // operand decoding like:
    //
    // [operand]
    //
    //----------------
    // 2 bits to indicate size [8,16,32,64] => 00,01,10,11  => Encoded in lower Operand???
    //
    //
    // 2 bits to indicate src      =>
    // 2 bits to indicate dst      =>
    // 4 bits to indicate register => d0..d7, a0..a7
    //
    // Example:
    //  move.b d0, 0x44  => 0x20, 0000|11|01, 0x44
    //      0x20 => 0010 | xx | 00
    //          0010 => Move class instr
    //          xx   => currently unused
    //          00   => size of operand -> 00 => byte sized
    //
    //      0000|11|01
    //          0000 => Destination register
    //            11 => Desintation is register (DstAddrMode)
    //            01 => Source is immediate (SrcAddrMode)
    //  move.d d5, 0x44  => 0x22, 0101|11|01, <32bit>
    //  move.l d3, 0x44  => 0x23, 0011|11|01, <64bit>
    //
    // [operand | sz addressing][dst | src/dst flags] [opt: flag | src]
    //
    //  move.l d1,(a3)   => 0x23, 0001|11|11, 0000|0011
    //
    //
    //
    typedef enum : uint8_t {
        Invalid  = 0,
        Immediate = 1,      // move.b d0, 0x44        <- move a byte to register d0, or: 'd0 = 0x44'
        Absolute =  2,      // move.b d0, [0x7ff001]  <- move a byte from absolute to register, 'd0 = *ptr'
        Register =  3,      // move.b d0, (a0)
        // Relative might not be needed
    } AddressMode;

    typedef enum : uint8_t {
        Byte,   // 8 bit
        Word,   // 16 bit
        DWord,  // 32 bit
        Long,   // 64 bit
    } OperandSize;


    typedef enum : uint8_t {
        BRK  = 0x00,

        MOV  = 0x20,
        PUSH = 0x30,
        ADD  = 0x40,
        POP  = 0x50,
        SUB  = 0x60,
        MUL  = 0x80,
        DIV  = 0xA0,
        CMP  = 0xC0,

        CALL = 0xD0,

        // CMP/ADD/SUB/MUL/DIV/MOV - will update zero
        BZC  = 0xE0,    // Branch Zero Clear
        BZS  = 0xE1,    // Branch Zero Set
        // ADD/SUB/MUL/DIV - will update carry
        BCC  = 0xE0,    // Branch Carry Clear
        BCS  = 0xE1,    // Branch Carry Set

        RET  = 0xF0,
        NOP  = 0xF1,

    } OperandClass;


    class VirtualCPU {
    public:
        VirtualCPU() = default;
        virtual ~VirtualCPU() = default;
        void Begin(void *ptrRam, size_t sizeofRam) {
            ram = static_cast<uint8_t *>(ptrRam);
            szRam = sizeofRam;
            // Everything is zero upon reset...
            memset(&registers, 0, sizeof(registers));
        }
        void Reset() {

        }
        void Step();

        const Registers &GetRegisters() {
            return registers;
        }

    protected:
        void ExecuteMoveInstr(OperandSize szOperand, AddressMode dstAddrMode, int idxDstRegister, AddressMode srcAddrMode);
    protected:
        // TODO: Can't really do this - need to control byte ordering HARD...
        template<typename T>
        T FetchFromInstrPtr() {
            auto address = registers.instrPointer.data.longword;
            if (address > szRam) {
                fmt::println(stderr, "Invalid address {}", address);
                exit(1);
            }
            T *ptrType = reinterpret_cast<T *>(&ram[address]);
            auto next = *ptrType;

            // Next address
            address += sizeof(T);
            registers.instrPointer.data.longword = address;

            return next;
        }
        uint8_t FetchByteFromInstrPtr() {
            return FetchFromInstrPtr<uint8_t>();
        }
        uint16_t FetchWordFromInstrPtr() {
            return FetchFromInstrPtr<uint16_t>();
        }
        uint32_t FetchDWordFromInstrPtr() {
            return FetchFromInstrPtr<uint32_t>();
        }
        uint64_t FetchLongFromInstrPtr() {
            return FetchFromInstrPtr<uint64_t>();
        }
    private:
        uint8_t *ram = nullptr;
        size_t szRam = 0;
        Registers registers = {};
        CPUFlags flags = {};
    };
}



#endif //VIRTUALCPU_H
