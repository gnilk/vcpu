//
// Created by gnilk on 09.04.24.
//

#ifndef VCPU_MESIBUSBASE_H
#define VCPU_MESIBUSBASE_H

#include <stdint.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

#include "BusBase.h"

namespace gnilk {
    namespace vcpu {

        // This implements the MESI part of the BusBase - but nothing else
        class MesiBusBase : public BusBase {
        public:
            using Ref = std::shared_ptr<MesiBusBase>;

            struct MemBusSnooper {
                uint8_t idCore = 0;
                MessageHandler cbOnMessage = nullptr;
            };

        public:
            MesiBusBase() = default;
            virtual ~MesiBusBase() = default;

            void Subscribe(uint8_t idCore, MessageHandler cbOnMessage) override;

            kMESIState BroadCastRead(uint8_t idCore, uint64_t addrDescriptor) override;
            void BroadCastWrite(uint8_t idCore, uint64_t addrDescriptor) override;

        protected:
            kMESIState SendMessage(kMemOp, uint8_t sender, uint64_t addrDescriptor);
        protected:
            size_t nextSubscriber = 0;
            std::array<MemBusSnooper, GNK_CPU_NUM_CORES> subscribers = {};
        };

    }
}

#endif //VCPU_MESIBUSBASE_H
