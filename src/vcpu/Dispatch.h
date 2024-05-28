//
// Created by gnilk on 28.05.24.
//

#ifndef VCPU_DISPATCH_H
#define VCPU_DISPATCH_H

#include <queue>
#include "InstructionDecoderBase.h"

namespace gnilk {
    namespace vcpu {
        struct DispatchItemBase {
        };

        template<size_t szRam>
        class Dispatch {
        public:
            Dispatch() = default;
            virtual ~Dispatch() = default;

            template<typename T>
            bool CanInsert() {
                if ((idxDataBuffer + sizeof(T)) > szRam) {
                    return false;
                }
                return true;
            }
            template<typename T>
            T &Next() {
                T *ptrCurrent = nullptr;
                if ((idxDataBuffer + sizeof(T)) > szRam) {
                    std::terminate();
                }

                ptrCurrent = (T *)&dataBuffer[idxDataBuffer];
                allocatedItems.push(ptrCurrent);
                idxDataBuffer += sizeof(T);
                return *ptrCurrent;

            }
            bool IsEmpty() {
                return allocatedItems.empty();
            }

            template<typename T>
            T &Pop() {
                auto ptrSomething = allocatedItems.front();
                allocatedItems.pop();
                return static_cast<T &>(*ptrSomething);
            }



        protected:
            std::queue<DispatchItemBase *> allocatedItems;


            uint8_t dataBuffer[szRam] = {};
            size_t idxDataBuffer = 0;

        };
    }
}


#endif //VCPU_DISPATCH_H
