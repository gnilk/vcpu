//
// Created by gnilk on 15.01.2024.
//

#ifndef PERIPHERAL_H
#define PERIPHERAL_H

namespace gnilk {
    namespace vcpu {
        class Peripheral {
        public:
            Peripheral() = default;
            virtual ~Peripheral() = default;

            virtual void Update() {}
        };
    }
}

#endif //PERIPHERAL_H
