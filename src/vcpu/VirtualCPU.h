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

    struct CPUStatusBits {
        uint8_t overflow : 1;
        uint8_t underflow : 1;
        uint8_t zero : 1;
        uint8_t unk1 : 1;
        uint8_t unk2 : 1;
        uint8_t unk3 : 1;
        uint8_t unk4 : 1;
        uint8_t unk5 : 1;
    };

    enum class CPUStatusFlags : uint8_t {
        None = 0,
        Overflow = 1,
        Underflow = 2,
        Zero = 4,
    };

    enum class CPUStatusFlagBitPos : uint8_t {
        Overflow = 0,
        Underflow = 1,
        Zero = 2,
    };

    union CPUStatusReg {
        CPUStatusBits flags;
        CPUStatusFlags eflags;
    };

    inline constexpr CPUStatusFlags operator | (CPUStatusFlags lhs, CPUStatusFlags rhs) {
        using T = std::underlying_type_t <CPUStatusFlags>;
        return static_cast<CPUStatusFlags>(static_cast<T>(lhs) | static_cast<T>(rhs));
    }

    inline constexpr CPUStatusFlags operator & (CPUStatusFlags lhs, CPUStatusFlags rhs) {
        using T = std::underlying_type_t <CPUStatusFlags>;
        return static_cast<CPUStatusFlags>(static_cast<T>(lhs) & static_cast<T>(rhs));
    }

    inline constexpr CPUStatusFlags operator ^ (CPUStatusFlags lhs, int rhs) {
        using T = std::underlying_type_t <CPUStatusFlags>;
        return static_cast<CPUStatusFlags>(static_cast<T>(lhs) ^ rhs);
    }


    inline CPUStatusFlags operator |= (CPUStatusFlags &lhs, CPUStatusFlags rhs) {
        lhs = lhs | rhs;
        return lhs;
    }

    inline constexpr CPUStatusFlags operator &= (CPUStatusFlags lhs, CPUStatusFlags rhs) {
        lhs = lhs & rhs;
        return lhs;
    }


    inline constexpr CPUStatusFlags operator << (bool bitvalue, CPUStatusFlagBitPos shiftvalue) {
        int res = (bitvalue?1:0) << static_cast<int>(shiftvalue);
        return static_cast<CPUStatusFlags>(res);
    }

    // Define some masks
    static constexpr CPUStatusFlags CPUStatusAritMask = CPUStatusFlags::Overflow | CPUStatusFlags::Underflow | CPUStatusFlags::Zero;
    static constexpr CPUStatusFlags CPUStatusAritInvMask = (CPUStatusFlags::Overflow | CPUStatusFlags::Underflow | CPUStatusFlags::Zero) ^ 0xff;



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
    // It is actually faster by some 10% to have more complicated instr. set...
    // However, I want the instruction set to be flexible and easier to handle - to I will
    //


    //
    // operand decoding like:
    //
    // Basic operand layout is 32 bits
    //
    //  byte | What
    //  ----------------------------
    //    0  | Op Code
    //    1  | Addressing size; bit 0,1 (0,1,2,3) -> 6 bits spare!
    //    2  | Dst Register and Flags: RRRR | FFFF => upper 4 bits 16 bit register index (0..7 => D0..D7, 8..15 => A0..A7)
    //       |  Flags: AddressMode flags (Immediate, Absolute, Register) - we have 2 bits spare!
    //    3  | Src Register and Flags: RRRR | FFFF => upper 4 bits 16 bit register index (0..7 => D0..D7, 8..15 => A0..A7)
    //       |  Flags: AddressMode flags (Immediate, Absolute, Register) - we have 2 bits spare!
    // ---------------------------------------------------------------------------
    // Note: Destination Address Mode 'Immediate' is invalid
    //
    // Depending on address mode (src/dst) the following differs
    //
    // SrcAddressMode: Immediate
    //  byte | What
    //  ----------------------------------
    //  0..n | Depedning on Address size 8..64 bits value follows
    //
    // Example:
    //  move.b d0, 0x44  => 0x20, 0x00, 0x03, 0x01, 0x44
    //      0x20 => move operand
    //      0x00 => Operand size: byte
    //      0x03 => Dst [Reg][Mode] => Reg = 0, Mode = Register
    //      0x01 => Src [Reg][Mode] => Reg = -, Mode = Immediate
    //      <8 bit> value
    //
    //   move.d d5, 0x44  => 0x20, 0x02, 0x53, 0x01,0x00, 0x00, 0x00, 0x44
    //
    //      0x20 => move operand
    //      0x02 => Operand Size: dword (32 bits)
    //      0x53 => Dst [Reg][Mode] => Reg = 5, Mode = Register
    //      0x01 => Src [Reg][Mode] => Reg = -, Mode = Immediate
    //      <32 bits> value
    //
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

    // high two bits of 'mode'
    typedef enum : uint8_t {
        None = 0,
        RegRelative = 1,
        RegRelativeShift = 2,
        AbsRelative = 3,
    } RelativeAddressing;

    typedef enum : uint8_t {
        Byte,   // 8 bit
        Word,   // 16 bit
        DWord,  // 32 bit
        Long,   // 64 bit
    } OperandSize;


    typedef enum : uint8_t {
        BRK  = 0x00,

        MOV  = 0x20,
        ADD  = 0x30,
        SUB  = 0x40,
        MUL  = 0x50,
        DIV  = 0x60,
        PUSH = 0x70,
        POP  = 0x80,
        CMP  = 0x90,

        CLC  = 0xA0,
        SEC  = 0xA1,

        AND = 0xB0,
        OR  = 0xB1,
        XOR = 0xB2,

        CALL = 0xC0,        // push next instr. on stack and jump
        JMP  = 0xC1,        // Jump
        SYS  = 0xC2,        // Issue a sys call



        // CMP/ADD/SUB/MUL/DIV/MOV - will update zero
        BZC  = 0xD0,    // Branch Zero Clear
        BZS  = 0xD1,    // Branch Zero Set
        // ADD/SUB/MUL/DIV - will update carry
        BCC  = 0xD2,    // Branch Carry Clear
        BCS  = 0xD3,    // Branch Carry Set

        RET  = 0xF0,    // pop addr. from stack and jump absolute..
        NOP  = 0xF1,
        EOC  = 0xff,        // End of code

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
            registers.instrPointer.data.longword = 0;
        }
        bool Step();

        const Registers &GetRegisters() const {
            return registers;
        }

        Registers &GetRegisters() {
            return registers;
        }

        const CPUStatusReg &GetStatusReg() const {
            return statusReg;
        }

    protected:
        void ExecuteMoveInstr(OperandSize szOperand, AddressMode dstAddrMode, int idxDstRegister, AddressMode srcAddrMode, int idxSrcRegister);
        void ExecuteAddInstr(OperandSize szOperand, AddressMode dstAddrMode, int idxDstRegister, AddressMode srcAddrMode, int idxSrcRegister);
        void ExecuteSubInstr(OperandSize szOperand, AddressMode dstAddrMode, int idxDstRegister, AddressMode srcAddrMode, int idxSrcRegister);
        void ExecuteMulInstr(OperandSize szOperand, AddressMode dstAddrMode, int idxDstRegister, AddressMode srcAddrMode, int idxSrcRegister);
        void ExecuteDivInstr(OperandSize szOperand, AddressMode dstAddrMode, int idxDstRegister, AddressMode srcAddrMode, int idxSrcRegister);

        RegisterValue ReadFromSrc(OperandSize szOperand, AddressMode srcAddrMode, int idxSrcRegister);
        RegisterValue ReadSrcImmediateMode(OperandSize szOperand);


    protected:
        template<typename T>
        T FetchFromInstrPtr() {
            auto address = registers.instrPointer.data.longword;
            if (address > szRam) {
                fmt::println(stderr, "Invalid address {}", address);
                exit(1);
            }
            T result = {};
            uint32_t nBits = 0;
            auto numToFetch = sizeof(T);
            while(numToFetch > 0) {
                auto byte = ram[address];
                address++;
                // this results in 'hex' dumps easier to read - MSB first (most significant byte first) - Intel?
                result = (result << nBits) | T(byte);
                nBits = 8;
                // this results in LSB - least significant byte first (Motorola?)
                // result |= T(byte) << nBits);
                // nBits += 8;
                numToFetch -= 1;
            }
            registers.instrPointer.data.longword = address;

            return result;
        }
        // This one is used alot and should be faster...
        __inline uint8_t FetchByteFromInstrPtr() {
            return FetchFromInstrPtr<uint8_t>();
/*
            auto address = registers.instrPointer.data.longword;
            auto byte = ram[address++];
            registers.instrPointer.data.longword = address;
            return byte;
*/
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
        CPUStatusReg statusReg = {};
    };
}



#endif //VIRTUALCPU_H
