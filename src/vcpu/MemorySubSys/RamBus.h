//
// Created by gnilk on 31.03.2024.
//

#ifndef VCPU_RAMBUS_H
#define VCPU_RAMBUS_H

#include <string>
#include <unordered_map>
#include <functional>
#include <stdint.h>
#include <memory>


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

        // this is my 'mmu' kind of
        class RamMemory {
        public:
            RamMemory() = delete;
            explicit RamMemory(size_t szRam);
            virtual ~RamMemory();

            // Well - we need something...
            void *TranslateAddress(void *ptr) {
                uint64_t dummy = (uint64_t)ptr;
                dummy = dummy & 0xffff;
                return (void *)dummy;
                //return &data[dummy];
            }

            // the raw pointers here - are 'HW' - ergo, they are not part of the emulated RAM - instead
            // they emulate the cache hardware buffers outside of the RAM address...
            void Write(uint64_t addrDescriptor, const void *src);
            void Read(void *dst, uint64_t addrDescriptor);

            // Emulation functions, use this to fetch an native address to the emulated RAM..
            // Note: This should mainly be used by unit testing and debugging to either inspect or place/modify
            //       values in the emulated RAM...
            void *PtrToRamAddress(uint64_t address) const {
                if (address > numBytes) {
                    return nullptr;
                }
                return &data[address];
            }
            size_t GetSizeInBytes() const {
                return numBytes;
            }
        private:
            size_t numBytes = 0;
            uint8_t *data = nullptr;
        };

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


        // RAM Bus
        class RamBus : public MesiBusBase {
        public:
            RamBus() = default; //(RamMemory &memory) : ram(memory) {}
            virtual ~RamBus() = default;

            static RamBus &Instance();

            // TEMP
            void SetRamMemory(RamMemory *memory) {
                ram = memory;
            }

            void WriteLine(uint64_t addrDescriptor, const void *src) override;
            void ReadLine(void *dst, uint64_t addrDescriptor) override;

            // Emulation helpers
            void *RamPtr(uint64_t address) const {
                return ram->PtrToRamAddress(address);
            }
        protected:
            // ???
            RamMemory *ram;
        };

    }
}

#endif //VCPU_RAMBUS_H
