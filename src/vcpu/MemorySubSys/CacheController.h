//
// Created by gnilk on 10.04.24.
//

#ifndef VCPU_CACHECONTROLLER_H
#define VCPU_CACHECONTROLLER_H

#include <stdint.h>
#include <type_traits>

#include "Cache.h"
#include "MesiBusBase.h"

namespace gnilk {
    namespace vcpu {
        // This is attached to each 'core' through the MMU
        // TO-DO:
        //   - Check the regional-flag if a memory address is cacheable
        class MMU;
        class Cache;

        class CacheController {
            friend MMU;
        public:
            CacheController() = default;
            virtual ~CacheController() = default;

            void Initialize(uint8_t coreIdentifier);

            void Touch(const uint64_t address);

            template<typename T>
            int32_t Write(uint64_t address, const T &value) {
                static_assert(std::is_integral_v<T> == true);
                return WriteInternalFromExternal(address, &value, sizeof(T));
            }

            template<typename T>
            T Read(uint64_t address) {
                static_assert(std::is_integral_v<T> == true);
                T value;
                ReadInternalToExternal(&value, address, sizeof(T));
                return value;
            }

            size_t Flush();

            const Cache& GetCache() {
                return cache;
            }
            int GetInvalidLineCount() const;
            void Dump() const;
        protected:

            kMESIState OnDataBusMessage(BusBase::kMemOp op, uint8_t sender, uint64_t addrDescriptor);
            int32_t ReadLine(BusBase::Ref bus, uint64_t addrDescriptor, kMESIState state);
            kMESIState OnMsgBusRd(uint64_t addrDescriptor);
            void OnMsgBusWr(uint64_t addrDescriptor);

        private:
            int32_t WriteInternalFromExternal(uint64_t address, const void *src, size_t nBytes);
            void ReadInternalToExternal(void *dst, const uint64_t address, size_t nBytes);

            void WriteMemory(const BusBase::Ref &bus, int idxLine);
            void ReadMemory(const BusBase::Ref &bus, int idxLine, uint64_t addrDescriptor, kMESIState state);

        private:
            uint8_t idCore = 0;
            Cache cache;
        };



    }
}

#endif //VCPU_CACHECONTROLLER_H
