//
// Created by gnilk on 28.05.24.
//

#ifndef VCPU_DISPATCH_H
#define VCPU_DISPATCH_H

#include <queue>
#include <mutex>

#include "Ringbuffer.h"

namespace gnilk {
    namespace vcpu {

        class DispatchBase {
        public:
            struct DispatchItemHeader {
                uint16_t szItem;        // Way overkill, one byte is probably more than enough - will test this out
                uint8_t  instrTypeId;   // Instruction Type ID - this is literally the extension byte
                uint8_t  reserved;      // alignment - and perhaps I need this anyway...
            };
        public:
            virtual bool IsEmpty() = 0;
            virtual bool CanInsert(size_t szItem) = 0;
            virtual int32_t Peek(DispatchItemHeader *outHeader) = 0;
            virtual bool Push(uint8_t instrTypeId, const void *item, size_t szItem) = 0;
            virtual bool Pop(void *ptrOut, size_t szItem) = 0;

        };
        //
        // Dispatcher queue with ring buffer
        //
        template<size_t szRam>
        class Dispatch : public DispatchBase {

        public:
            Dispatch() = default;
            virtual ~Dispatch() = default;

            //
            // Peek
            // Returns
            //       0  - empty
            //      -1 - error, queue is corrupt
            //       1 - one item available
            //
            int32_t Peek(DispatchItemHeader *outHeader) override {
                std::lock_guard guard(lock);
                if (IsEmpty()) {
                    return 0;
                }
                // This should not be the case - but better safe than sorry..
                if (ringbuffer.BytesAvailable() <= sizeof(DispatchItemHeader)) {
                    return -1;
                }
                if (ringbuffer.Peek(outHeader, sizeof(DispatchItemHeader)) < 0) {
                    return -1;
                }
                return 1;
            }


            bool IsEmpty() override {
                return (ringbuffer.BytesAvailable() == 0);
            }

            bool CanInsert(size_t szItem) override {
                if (ringbuffer.BytesFree() <= szItem) {
                    return false;
                }
                return true;
            }


            bool Push(uint8_t instrTypeId, const void *item, size_t szItem) override {
                std::lock_guard guard(lock);

                if (ringbuffer.BytesFree() < (szItem + sizeof(DispatchItemHeader))) {
                    return false;
                }
                if (szItem > std::numeric_limits<uint16_t>::max()) {
                    return false;
                }
                DispatchItemHeader header = {
                        .szItem = static_cast<uint16_t>(szItem),
                        .instrTypeId = instrTypeId,
                        .reserved = 0,

                };
                if (ringbuffer.Write(&header, sizeof(header)) < 0) {
                    return false;
                }
                if (ringbuffer.Write(item, szItem) < 0) {
                    return false;
                }

                return true;

            }

            bool Pop(void *ptrOut, size_t szItem) override {
                std::lock_guard guard(lock);

                if (ringbuffer.BytesAvailable() < (szItem + sizeof(DispatchItemHeader))) {
                    return false;
                }
                DispatchItemHeader header;
                if (ringbuffer.Read(&header, sizeof(header)) < 0) {
                    return false;
                }
                if (header.szItem != szItem) {
                    // We are corrupt!!!
                    return false;
                }
                if (ringbuffer.Read(ptrOut, szItem) < 0) {
                    return false;
                }

                return true;
            }
        protected:
            std::mutex lock;
            Ringbuffer<szRam> ringbuffer;
        };
    }
}


#endif //VCPU_DISPATCH_H
