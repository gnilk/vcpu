//
// Created by gnilk on 09.04.24.
//

#ifndef VCPU_FLASHBUS_H
#define VCPU_FLASHBUS_H

#include "BusBase.h"

namespace gnilk {
    namespace vcpu {

        class FlashBus : public BusBase {
        public:
            FlashBus() = default; //(RamMemory &memory) : ram(memory) {}
            virtual ~FlashBus() = default;

            static BusBase::Ref Create(RamMemory *ptrRam);
            static BusBase::Ref Create(size_t szRam);

            void SetRamMemory(RamMemory *memory) {
                ram = memory;
            }

            // I need this!
            void ReadData(void *dst, uint64_t addrDescriptor, size_t nBytes);
            void WriteData(uint64_t addrDescriptor, const void *src, size_t nBytes);

        protected:
            RamMemory *ram;

        };
    }
}


#endif //VCPU_FLASHBUS_H
