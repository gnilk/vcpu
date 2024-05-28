//
// Created by gnilk on 28.05.24.
//

#ifndef VCPU_DISPATCH_H
#define VCPU_DISPATCH_H

#include <queue>
#include <mutex>

#include "InstructionDecoderBase.h"
#include "Ringbuffer.h"

namespace gnilk {
    namespace vcpu {

        //
        // Dispatcher queue with ring buffer
        //
        template<size_t szRam>
        class Dispatch {
        protected:
            struct DispatchItemHeader {
                uint16_t szItem;        // Way overkill, one byte is probably more than enough - will test this out
            };

        public:
            Dispatch() = default;
            virtual ~Dispatch() = default;

            bool IsEmpty() {
                return (ringbuffer.BytesAvailable() == 0);
            }

            template<typename T>
            bool CanInsert() {
                if (ringbuffer.BytesFree() <= sizeof(T)) {
                    return false;
                }
                return true;
            }

            template<typename T>
            bool Push(const T &item) {
                std::lock_guard guard(lock);

                if (ringbuffer.BytesFree() < (sizeof(T) + sizeof(DispatchItemHeader))) {
                    return false;
                }
                DispatchItemHeader header = { .szItem = sizeof(T) };
                if (ringbuffer.Write(&header, sizeof(header)) < 0) {
                    return false;
                }
                if (ringbuffer.Write(&item, sizeof(T)) < 0) {
                    return false;
                }

                return true;

            }

            template<typename T>
            bool Pop(T *ptrOut) {
                std::lock_guard guard(lock);

                if (ringbuffer.BytesAvailable() < (sizeof(T) + sizeof(DispatchItemHeader))) {
                    return false;
                }
                DispatchItemHeader header;
                if (ringbuffer.Read(&header, sizeof(header)) < 0) {
                    return false;
                }
                if (header.szItem != sizeof(T)) {
                    // We are corrupt!!!
                    return false;
                }
                if (ringbuffer.Read(ptrOut, sizeof(T)) < 0) {
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
