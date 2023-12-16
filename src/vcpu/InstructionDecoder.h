//
// Created by gnilk on 16.12.23.
//

#ifndef INSTRUCTIONDECODER_H
#define INSTRUCTIONDECODER_H

#include <stdint.h>
#include <memory>
#include "InstructionSet.h"
#include "CPUBase.h"

namespace gnilk {
    namespace vcpu {

        class InstructionDecoder {
        public:
            using Ref = std::shared_ptr<InstructionDecoder>;
        public:
            InstructionDecoder() = default;             // I know it is possible to hide this...
            virtual ~InstructionDecoder() = default;
            static InstructionDecoder::Ref Create(OperandClass opClass);

            bool Decode(CPUBase &cpu);

        public:
            // Used during by decoder...
            OperandClass opClass;
            OperandDescription description;

            OperandSize szOperand; // Only if 'description.features & OperandSize' == true
            uint8_t dstRegAndFlags; // Always set
            uint8_t srcRegAndFlags; // Only if 'description.features & TwoOperands' == true

            AddressMode dstAddrMode; // Always decoded from 'dstRegAndFlags'
            uint8_t dstRegIndex; // decoded like: (dstRegAndFlags>>4) & 15;

            // Only if 'description.features & TwoOperands' == true
            AddressMode srcAddrMode; // decoded from srcRegAndFlags
            uint8_t srcRegIndex; // decoded like: (srcRegAndFlags>>4) & 15;

        };
    }
}



#endif //INSTRUCTIONDECODER_H
