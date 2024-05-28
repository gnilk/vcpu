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

#include "InstructionSetDefBase.h"

namespace gnilk {
    namespace vcpu {



        //
        // It is actually faster by some 10% to have more complicated instr. set...
        // However, I want the instruction set to be flexible and easier to handle
        //

        // TO-DO:
        // - extend this with atomic stuff (use lc/sr or cas?; https://en.wikipedia.org/wiki/Load-link/store-conditional)
        //      - do I need atomic swap and such?
        // - 'fence' to sync pipeline
        // - 'hint' or other instructions to read performance values or update them; see RISC-V ISA
        // - Need to verify my core control block - most likely must extend this (see RISC-V ISA, Chapter 10 - they have 4096 CSR reg's)
        //   also see: file:///home/gnilk/Downloads/priv-isa-asciidoc.pdf
        // - might need something to handle the mmu explicitly...

        //
        // Op-Codes are either 1,3 or 4 bytes..
        // 1 byte - NOP, RET, RTI, CLC, SEC, etc... which operate on predefined flags or simply don't have an argument
        // 3 byte - single operand code, with opsize and family; call
        // 4 byte - double operand code, with opsize and family; move, lea, add, sub, etc...
        //
        // Basic two-operand mnemonic layout is 32 bits, like: move <op1>,<op2>
        // Relative addressing adds one or two bytes for relative modes. It is possible to have 'move (a0+d1<<1),(a1+d2<<2)'
        //
        //
        //  byte | What
        //  ----------------------------
        //    0  | Op Code
        //    1  | OpSizeAndFamily (rrFF | rrSS)
        //           SS, Size bits 0,1 (0,1,2,3)
        //           FF, Family bits [0 - integer, 1 - control, 2 - floating]   <- floating can be deprecated, fpu handling through instr.extensions
        //           rr, Reserved
        //    2  | Dst Register and Flags: RRRR | FFFF => upper 4 bits register index (0..7 => D0..D7, 8..15 => A0..A7)
        //       |  Flags [FFFF] -> [RR|AA]
        //       |      RR : Relative Addressing (None, RegRelative, Absolute, <unused>)
        //       |      AA : Address Mode (Indirect, Immediate, Absolute, Register)
        //       |
        //    3  | Src Register and Flags: RRRR | FFFF => upper 4 bits 16 bit register index (0..7 => D0..D7, 8..15 => A0..A7)
        //       |  Flags [FFFF] -> [RR|AA]
        //       |      RR : Relative Addressing (None, RegRelative, Absolute, <unused>)
        //       |      AA : Address Mode (Indirect, Immediate, Absolute, Register)
        //
        // [4..5]| if Relative Addressing set (RegRelative or Absolute) another byte will follow for each operand with rel. addressing
        //       |    if RegRelative => RRRR | SSSS, R = RegIndex, S => Shift
        //       |    if Absolute   => <offset>
        //
        //   ..  | Absolute or Immediate value
        //
        // ---------------------------------------------------------------------------
        // Note: Destination Address Mode 'Immediate' is invalid
        //
        // Depending on address mode (src/dst) the following differs
        //
        // SrcAddressMode: Immediate
        //  byte | What
        //  ----------------------------------
        //  0..n | Depending on Address size 8..64 bits value follows
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


        // Operand Size has 1 byte and the following layout
        //  OpSize: rrFF | rrSS
        // where:
        //    rr - reserved, 2 bits
        //    FF - operand family or class
        //    rr - reserved, 2 bits
        //    SS - size
        // Perhaps: Add one bit for 'zero extend'?
        //  OpSize: xxxx | xZSS  ?
        // Perhaps add a family of instructions which always have <byte> size operand
        // specifically bit-shift operands where currently if I want to shift a DWORD with an immediate value
        // that value will occupy 32 bits (as src/dst operands must have same size)
        // However, I want the decoder to as simple as possible and without look-up tables..
        // So best would be to reserve a range:




        //
        // Consider special instruction for
        // - copy exception control block to memory address, either this is to a pre-reserved area
        //
        //      like:
        //          cpctrl    [mask]            <- after copy, CR7 contains address of ctrl-block
        //          move.l    a0,cr7            <- get a local copy of CR7
        //          ; need to disable mmu before this...
        //          move.l    d0,(a0+exp.mask)  <- read data out of exception ctrl block
        //
        //      or (this requires CPU to copy via MMU)
        //          lea     a0, my_ctrl_block_area
        //          cpctrl  [mask]                  <- copy to a0
        //          move.l  d0, (a0+exp.mask)
        //

        // These are the op-codes, there is plenty of 'air' inbetween the numbers - no real thought went in to the
        // numbers as such. The opcodes are 8-bit..
        // Details for each op-code is in the 'InstructionSet.cpp' file..
        typedef enum : OperandCodeBase {
            BRK = 0x00,

            MOV = 0x20,     // one op-code for all 'move' instructions
            LEA = 0x28,
            ADD = 0x30,
            SUB = 0x40,
            MUL = 0x50,
            DIV = 0x55,

            // TODO: Relocate these to 0x60 -> requires fixing quite a lot of unit-tests
            //       However,extension testing becomes easy and we just have to test if (byte & 0xf0) => Extension
            //       And op on 4 bits in the decoder stage is very simple...
            RET = 0x60,
            // pop addr. from stack and jump absolute..
            NOP = 0x61,
            RTI = 0x62, // Return from Interrupt
            RTE = 0x63, // Return from Exception


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
            BEQ = 0xD0, // Branch Equal - Zero Set
            BNE = 0xD1, // Branch Not Equal - Zero Clear
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

            // Extension instr. space [0x60..0x6f]
            FP16 = 0xF0,        // half precision 16bit floating point instructions, IEEE 754-2008
            FP32 = 0xF1,        // 32bit floating point instruction, IEEE 754-2008

            EXT = 0xF6,         // Extension of instr. set - not yet decided how I want this to work..
            SIMD = 0xF8,        // vector instructions?

            EOC = 0xff,
            // End of code
        } OperandCode;

        static const uint8_t OperandCodeExtensionMask = 0xf0;

        class InstructionSetV1Def : public InstructionSetDefBase {
        public:
            struct RelativeAddressing {
                RelativeAddressMode mode;
                // not sure this is a good idea
                union {
                    uint8_t absoulte;
                    struct {
                        uint8_t shift : 4;      // LSB
                        uint8_t index : 4;      // MSB
                    } reg;
                } relativeAddress;
            };

        public:
            const std::unordered_map<OperandCodeBase, OperandDescriptionBase> &GetInstructionSet() override;
            std::optional<OperandDescriptionBase> GetOpDescFromClass(OperandCodeBase opClass) override;
            std::optional<OperandCodeBase> GetOperandFromStr(const std::string &str) override;
            // uint8_t EncodeOpSizeAndFamily(OperandSize opSize, OperandFamily opFamily) override;
        };

    }
}

#endif //INSTRUCTIONSET_H
