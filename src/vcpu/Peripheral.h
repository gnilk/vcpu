//
// Created by gnilk on 15.01.2024.
//

#ifndef PERIPHERAL_H
#define PERIPHERAL_H

#include <memory>

namespace gnilk {
    namespace vcpu {
        class Peripheral {
        public:
            using Ref = std::shared_ptr<Peripheral>;
        public:
            Peripheral() = default;
            virtual ~Peripheral() = default;

            virtual void Initialize() {}
            virtual bool Update() { return false; }
        };
    }
}

#endif //PERIPHERAL_H
