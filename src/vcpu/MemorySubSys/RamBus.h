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

#include "MesiBusBase.h"

namespace gnilk {
    namespace vcpu {






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
