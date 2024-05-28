//
// Created by gnilk on 25.05.24.
//

#ifndef VCPU_INSTRUCTIONSETDEFBASE_H
#define VCPU_INSTRUCTIONSETDEFBASE_H

#include <string>
#include <stdint.h>
#include <unordered_map>
#include <optional>

#include "BitmaskFlagOperators.h"

namespace gnilk {
    namespace vcpu {
        //
        // These are shared by all types of instructions..
        //
        struct OperandDescriptionBase {
            std::string name;
            uint32_t features;
        };
        typedef uint8_t OperandCodeBase;

        // FIXME: Make generic and the decoder should translate any internal to generic versions
        typedef enum : uint8_t {
            Byte  = 0,   // 8 bit
            Word  = 1,   // 16 bit
            DWord = 2,  // 32 bit
            Long  = 3,   // 64 bit
        } OperandSize;

        // Class/Target of operand
        typedef enum : uint8_t {
            Integer = 0,
            Control = 1,
            Float = 2,
        } OperandFamily;

        // Generic type of address-modes
        typedef enum : uint8_t {
            Indirect = 0,       // move.b d0, (a0)
            Immediate = 1,      // move.b d0, 0x44        <- move a byte to register d0, or: 'd0 = 0x44'
            Absolute = 2,       // move.b d0, [0x7ff001]  <- move a byte from absolute to register, 'd0 = *ptr'
            Register = 3,       // move.b d0, d1
        } AddressMode;

        // Geneeric sub-type of address-mode
        typedef enum : uint8_t {
            None = 0,
            RegRelative = 1,        // move.b d0, (a0 + d0)
            AbsRelative = 2,        // move.b d0, (a0 + <const>)
            Unused = 3,             //
        } RelativeAddressMode;


        //
        // This feature description and table should be made visible across all CPU tools..
        // Put it in a a separate place so it can be reused...
        //
        typedef enum : uint32_t {
            // Has operand size distinction
            kFeature_OperandSize = 1,
            // Two operand bits, allows for 0..3 operands
            kFeature_OneOperand = 2,
            kFeature_TwoOperands = 4,
            kFeature_ThreeOperands = 8,
            // Support immediate values; move d0, #<value>
            kFeature_Immediate = 0x10,
            // Support any register
            kFeature_AnyRegister = 0x20,
            // Support address register only
            kFeature_AddressRegister = 0x40,
            // Support addressing move d0, (a0)
            kFeature_Addressing = 0x80,        // Is this not just 'register'??

            // FIXME: This is not branching, this defines how the instruction is to treat reading a label value
            //        IF present, the instruction will USE the memory address and perform the operation (branch, load-of-address, etc)
            //        if NOT present, the CPU will fetch the memory of the address and the value stored at the address...
            //        consider:
            //              move.l d0, variable         ; D0 will have the value of the variable (stored at some address - i.e. the CPU will READ from the address)
            //        whereby:
            //              call variable               ; the CPU will USE the address directly
            //              lea.l d0, variable          ; D0 will contain the ADDRESS (and not the value of the variable)
            //
            // Maybe we should rename this as 'Absolute'?
            //
            kFeature_Branching = 0x100,
            // Control instruction - can have the 'Control' bit of the Family in in the OpSize byte set
            kFeature_Control = 0x200,
            // FIXME: Perhaps need absolute as well..
            kFeature_Extension = 0x400,    // Extension prefix

            // Various extension flags
            kFeature_Mask    = 0x1000,
            kFeature_Advance = 0x2000,
        } OperandFeatureFlags;

        template<>
        struct EnableBitmaskOperators<OperandFeatureFlags> {
            static const bool enable = true;
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


        //typedef uint8_t OperandCode;

        class InstructionSetDefBase {
        public:
            virtual const std::unordered_map<OperandCodeBase , OperandDescriptionBase> &GetInstructionSet() = 0;
            virtual std::optional<OperandDescriptionBase> GetOpDescFromClass(OperandCodeBase opClass) = 0;
            virtual std::optional<OperandCodeBase> GetOperandFromStr(const std::string &str) = 0;
//            virtual uint8_t EncodeOpSizeAndFamily(OperandSize opSize, OperandFamily opFamily) {
//                return 0;
//            }
        };
    }
}
#endif //VCPU_INSTRUCTIONSETDEFBASE_H
