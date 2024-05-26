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
