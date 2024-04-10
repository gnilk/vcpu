//
// Created by gnilk on 10.04.24.
//

#ifndef VCPU_HWMAPPEDBUS_H
#define VCPU_HWMAPPEDBUS_H

#include "BusBase.h"

namespace gnilk {
    namespace vcpu {
        //
        // This (by default) assigned to memory regions with 'HWMapping' flag set
        // It is a callback based bus - i.e. the emulator/caller defines what goes where..
        // As such, you must assign the Read/Write handlers
        //
        class HWMappedBus : public BusBase {
        public:
            using ReadHandler = std::function<void(void *dst, uint64_t address, size_t nBytes)>;
            using WriteHandler = std::function<void(uint64_t address, const void *src, size_t nBytes)>;
        public:
            HWMappedBus() = default; //(RamMemory &memory) : ram(memory) {}
            virtual ~HWMappedBus() = default;

            static BusBase::Ref Create();

            void SetReadWriteHandlers(ReadHandler newOnRead, WriteHandler newOnWrite) {
                onRead = newOnRead;
                onWrite = newOnWrite;
            }

            // I need this!
            void ReadData(void *dst, uint64_t addrDescriptor, size_t nBytes);
            void WriteData(uint64_t addrDescriptor, const void *src, size_t nBytes);
        protected:
            ReadHandler  onRead = nullptr;
            WriteHandler onWrite = nullptr;
        };

    }
}


#endif //VCPU_HWMAPPEDBUS_H
