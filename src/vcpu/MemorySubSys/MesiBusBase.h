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

namespace gnilk {
    namespace vcpu {
// Try make this a runtime config -> need to replace array with vector...
#ifndef GNK_CPU_NUM_CORES
#define GNK_CPU_NUM_CORES 4
#endif

#ifndef GNK_L1_CACHE_LINE_SIZE
#define GNK_L1_CACHE_LINE_SIZE 64
#endif
#ifndef GNK_L1_CACHE_NUM_LINES
#define GNK_L1_CACHE_NUM_LINES 4
#endif

#define GNK_ADDR_DESC_FROM_ADDR(__addr__) (uint64_t(__addr__) & ~(GNK_L1_CACHE_LINE_SIZE-1))
#define GNK_LINE_OFS_FROM_ADDR(__addr__) (uint64_t(__addr__) & (GNK_L1_CACHE_LINE_SIZE-1))


        enum kMESIState {
            kMesi_Modified = 0x01,
            kMesi_Exclusive = 0x02,
            kMesi_Shared = 0x04,
            kMesi_Invalid = 0x08,
        };
        static const std::string &MESIStateToString(kMESIState state) {
            static std::unordered_map<kMESIState, std::string> stateNames = {
                    {kMesi_Modified, "Modified"},
                    {kMesi_Exclusive, "Exclusive"},
                    {kMesi_Shared, "Shared"},
                    {kMesi_Invalid, "Invalid"},
            };
            return stateNames[state];
        }
        class MesiBusBase {
        public:
            using Ref = std::shared_ptr<MesiBusBase>;
            // See: https://en.wikipedia.org/wiki/MESI_protocol
            enum class kMemOp {
                kProcRd,    // Processor Read Request
                kProcWr,    // Processor Write Request
                kBusRd,     // Bus requesting to read memory
                kBusWr,     // Bus requesting to write memory
                kBusRdX,    // Bus requesting to write memory to cache
            };
            using MessageHandler = std::function<kMESIState(kMemOp op, uint8_t sender, uint64_t addrDescriptor)>;

            struct MemBusSnooper {
                uint8_t idCore = 0;
                MessageHandler cbOnMessage = nullptr;
            };

        public:
            MesiBusBase() = default;
            virtual ~MesiBusBase() = default;

            void Subscribe(uint8_t idCore, MessageHandler cbOnMessage);
            kMESIState SendMessage(kMemOp, uint8_t sender, uint64_t addrDescriptor);

            kMESIState BroadCastRead(uint8_t idCore, uint64_t addrDescriptor);
            void BroadCastWrite(uint8_t idCore, uint64_t addrDescriptor);


            virtual void WriteLine(uint64_t addrDescriptor, const void *src) = 0;
            virtual void ReadLine(void *dst, uint64_t addrDescriptor) = 0;
        protected:
            size_t nextSubscriber = 0;
            std::array<MemBusSnooper, GNK_CPU_NUM_CORES> subscribers = {};
        };

    }
}

#endif //VCPU_MESIBUSBASE_H
