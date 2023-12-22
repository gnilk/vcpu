//
// Created by gnilk on 16.12.23.
//

#ifndef INSTRUCTIONSET_H
#define INSTRUCTIONSET_H

#include <stdint.h>
#include <type_traits>
#include <unordered_map>
#include <string>
#include <optional>

namespace gnilk {
    namespace vcpu {
        //
        // It is actually faster by some 10% to have more complicated instr. set...
        // However, I want the instruction set to be flexible and easier to handle - to I will
        //


        //
        // operand decoding like:
        //
        // Basic two-operand mnemonic layout is 32 bits, like: move <op1>,<op2>
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
            Indirect = 0,       // move.b d0, (a0)
            Immediate = 1,      // move.b d0, 0x44        <- move a byte to register d0, or: 'd0 = 0x44'
            Absolute = 2,       // move.b d0, [0x7ff001]  <- move a byte from absolute to register, 'd0 = *ptr'
            Register = 3,       // move.b d0, d1
        } AddressMode;

        // high two bits of 'mode'
        typedef enum : uint8_t {
            None = 0,
            RegRelative = 1,        // move.b d0, (a0 + d0)
            AbsRelative = 2,        // move.b d0, (a0 + <const>)
            Unused = 3,             // ?? move.b d0, (a0 + d0<<1) ??
        } RelativeAddressMode;

        // Operand Size has 1 byte and we only use the two lowest bits
        //  OpSize: xxxx | xxss
        // Perhaps: Add one bit for 'zero extend'?
        //  OpSize: xxxx | xZSS  ?
        // Perhaps add a family of instructions which always have <byte> size operand
        // specifically bit-shift operands where currently if I want to shift a DWORD with an immediate value
        // that value will occupy 32 bits (as src/dst operands must have same size)
        // However, I want the decoder to as simple as possible and without look-up tables..
        // So best would be to reserve a range:
        enum class OperandSize : uint8_t {
            Byte,   // 8 bit
            Word,   // 16 bit
            DWord,  // 32 bit
            Long,   // 64 bit

        };

        static constexpr size_t ByteSizeOfOperandSize(OperandSize opSize) {
            switch(opSize) {
                case OperandSize::Byte :
                    return sizeof(uint8_t);
                case OperandSize::Word :
                    return sizeof(uint16_t);
                case OperandSize::DWord :
                    return sizeof(uint32_t);
                case OperandSize::Long :
                default:
                    return sizeof(uint64_t);
            }
        }


        // These are the op-codes, there is plenty of 'air' inbetween the numbers - no real thought went in to the
        // numbers as such. The opcodes are 8-bit..
        // Details for each op-code is in the 'InstructionSet.cpp' file..
        typedef enum : uint8_t {
            BRK = 0x00,

            MOV = 0x20,     // one op-code for all 'move' instructions
            LEA = 0x28,
            ADD = 0x30,
            SUB = 0x40,
            MUL = 0x50,
            DIV = 0x60,
            PUSH = 0x70,
            POP = 0x80,
            CMP = 0x90,

            CLC = 0xA0,
            SEC = 0xA1,

            AND = 0xB0,
            OR = 0xB1,
            XOR = 0xB2,

            CALL = 0xC0,        // push next instr. on stack and jump
            JMP = 0xC1,         // Jump
            SYS = 0xC2,         // Issue a sys call, index (word) in d0, other arguments per syscall


            // CMP/ADD/SUB/MUL/DIV/MOV - will update zero
            BZC = 0xD0, // Branch Zero Clear
            BZS = 0xD1, // Branch Zero Set
            // ADD/SUB/MUL/DIV - will update carry
            BCC = 0xD2, // Branch Carry Clear
            BCS = 0xD3, // Branch Carry Set

            // The Ex family is special - their dst operand is also the src operand is the operate value
            // the operate value is also locked to byte-access...
            // Thus,
            //      LSR.l dst/src, <imm><reg><deref>
            //  .l affects only the dst/src while the imm/reg/deref will be byte-access
            // Reason: There is no point shifting a value more than 255 bits!

            LSR = 0xE3, // Logical Shift Left, 0 will be shifted (or carry??)
            ASR = 0xE4, // Arithmetic Shift Right, CARRY will be shifted out, sign will be preserved..
            LSL = 0xE5, // Logical Shift Right, 0 will be shifted in (or carry??)
            ASL = 0xE6, // Arithmetic Shift Left, CARRY will be shifted out, sign will be preserved..


            RET = 0xF0,
            // pop addr. from stack and jump absolute..
            NOP = 0xF1,
            EOC = 0xff,
            // End of code
        } OperandClass;


        //
        // This feature description and table should be made visible across all CPU tools..
        // Put it in a a separate place so it can be reused...
        //
        enum class OperandDescriptionFlags : uint8_t {
            // Has operand size distinction
            OperandSize = 1,
            // Two operand bits, allows for 0..3 operands
            OneOperand = 2,
            TwoOperands = 4,
            // Support immediate values; move d0, #<value>
            Immediate = 8,
            // Support register
            Register = 16,
            // Support addressing move d0, (a0)
            Addressing = 32,        // Is this not just 'register'??

            Branching = 64
        };

        inline constexpr OperandDescriptionFlags operator |(OperandDescriptionFlags lhs, OperandDescriptionFlags rhs) {
            using T = std::underlying_type_t<OperandDescriptionFlags>;
            return static_cast<OperandDescriptionFlags>(static_cast<T>(lhs) | static_cast<T>(rhs));
        }

        inline constexpr bool operator &(OperandDescriptionFlags lhs, OperandDescriptionFlags rhs) {
            using T = std::underlying_type_t<OperandDescriptionFlags>;
            return (static_cast<T>(lhs) & static_cast<T>(rhs));
        }

        struct OperandDescription {
            std::string name;
            OperandDescriptionFlags features;
        };

        const std::unordered_map<OperandClass, OperandDescription> &GetInstructionSet();
        std::optional<OperandDescription> GetOpDescFromClass(OperandClass opClass);
        std::optional<OperandClass> GetOperandFromStr(const std::string &str);

    }
}

#endif //INSTRUCTIONSET_H
