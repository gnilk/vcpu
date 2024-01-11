//
// Created by gnilk on 20.12.23.
//

#ifndef SEGMENT_H
#define SEGMENT_H

#include <stdint.h>

#include <vector>
#include <string>

#include "InstructionSet.h"

namespace gnilk {
    namespace assembler {
        class CompiledUnit;

        class Segment {
            friend CompiledUnit;
        public:
            using Ref = std::shared_ptr<Segment>;
        public:
            Segment(const std::string &segName, uint64_t address) : name(segName), loadAddress(address) {}
            virtual ~Segment() = default;

            const std::vector<uint8_t> &Data() const {
                return data;
            }

            const void *DataPtr() const {
                return data.data();
            }

            size_t Size() const {
                return data.size();
            }

            uint64_t LoadAddress() const {
                return loadAddress;
            }

            void SetLoadAddress(uint64_t newLoadAddress) {
                loadAddress = newLoadAddress;
            }

            const std::string &Name() const {
                return name;
            }

            void WriteByte(uint8_t byte) {
                data.push_back(byte);
            }
            void ReplaceAt(uint64_t offset, uint64_t newValue) {
                if (data.size() < 8) {
                    return;
                }
                if (offset > (data.size() - 8)) {
                    return;
                }

                data[offset++] = (newValue>>56 & 0xff);
                data[offset++] = (newValue>>48 & 0xff);
                data[offset++] = (newValue>>40 & 0xff);
                data[offset++] = (newValue>>32 & 0xff);
                data[offset++] = (newValue>>24 & 0xff);
                data[offset++] = (newValue>>16 & 0xff);
                data[offset++] = (newValue>>8 & 0xff);
                data[offset++] = (newValue & 0xff);
            }
            void ReplaceAt(uint64_t offset, uint64_t newValue, vcpu::OperandSize opSize) {
                if (data.size() < vcpu::ByteSizeOfOperandSize(opSize)) {
                    return;
                }
                if (offset > (data.size() - vcpu::ByteSizeOfOperandSize(opSize))) {
                    return;
                }
                switch(opSize) {
                    case vcpu::OperandSize::Byte :
                        data[offset++] = (newValue & 0xff);
                        break;
                    case vcpu::OperandSize::Word :
                        data[offset++] = (newValue>>8 & 0xff);
                        data[offset++] = (newValue & 0xff);
                        break;
                    case vcpu::OperandSize::DWord :
                        data[offset++] = (newValue>>24 & 0xff);
                        data[offset++] = (newValue>>16 & 0xff);
                        data[offset++] = (newValue>>8 & 0xff);
                        data[offset++] = (newValue & 0xff);
                        break;
                    default:
                        return;
                }
            }


            uint64_t GetCurrentWritePtr() {
                return data.size();
            }
        protected:
            std::string name = {};
            uint64_t loadAddress = 0;
            std::vector<uint8_t> data;
        };

    }
}

#endif //SEGMENT_H
