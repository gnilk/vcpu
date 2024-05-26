//
// Created by gnilk on 25.05.24.
//

#ifndef VCPU_INSTRUCTIONSETDEFBASE_H
#define VCPU_INSTRUCTIONSETDEFBASE_H

#include <string>
#include <stdint.h>
#include <unordered_map>
#include <optional>

namespace gnilk {
    namespace vcpu {
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

        //typedef uint8_t OperandCode;

        class InstructionSetDefBase {
        public:
            virtual const std::unordered_map<OperandCodeBase , OperandDescriptionBase> &GetInstructionSet() = 0;
            virtual std::optional<OperandDescriptionBase> GetOpDescFromClass(OperandCodeBase opClass) = 0;
            virtual std::optional<OperandCodeBase> GetOperandFromStr(const std::string &str) = 0;

        };
    }
}
#endif //VCPU_INSTRUCTIONSETDEFBASE_H
