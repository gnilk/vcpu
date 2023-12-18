//
// Created by gnilk on 16.12.23.
//

#ifndef CPUBASE_H
#define CPUBASE_H

#include <stdint.h>
#include <type_traits>
#include <stack>

#include "fmt/format.h"

#include "InstructionSet.h"

namespace gnilk {
    namespace vcpu {
        struct RegisterValue {
            union {
                uint8_t byte;
                uint16_t word;
                uint32_t dword;
                uint64_t longword;
            } data;
            // Make sure this is zero when creating a register..
            RegisterValue() {
                data.longword = 0;
            }

            explicit RegisterValue(uint8_t v) { data.byte = v; }
            explicit RegisterValue(uint16_t v) { data.word = v; }
            explicit RegisterValue(uint32_t v) { data.dword = v; }
            explicit RegisterValue(uint64_t v) { data.longword = v; }
        };

        // FIXME: Copy M68k status bits...
        struct CPUStatusBits {
            uint8_t carry : 1;
            uint8_t overflow : 1;
            uint8_t zero : 1;
            uint8_t negative : 1;
            uint8_t extend : 1;
            uint8_t int1 : 1;
            uint8_t int2 : 1;
            uint8_t int3 : 1;
        };

        enum class CPUStatusFlags : uint8_t {
            None = 0,
            Carry = 1,
            Overflow = 2,
            Zero = 4,
            Negative = 8,
            Extend = 16,
            Int1 = 32,
            Int2 = 64,
            Int3 = 128
        };

        enum class CPUStatusFlagBitPos : uint8_t {
            Carry = 0,
            Overflow = 1,
            Zero = 2,
            Negative = 3,
            Extend = 4,
            Int1 = 5,
            Int2 = 6,
            Int3 = 7,
        };

        union CPUStatusReg {
            CPUStatusBits flags;
            CPUStatusFlags eflags;
        };

        inline constexpr CPUStatusFlags operator | (CPUStatusFlags lhs, CPUStatusFlags rhs) {
            using T = std::underlying_type_t <CPUStatusFlags>;
            return static_cast<CPUStatusFlags>(static_cast<T>(lhs) | static_cast<T>(rhs));
        }

        inline constexpr CPUStatusFlags operator & (CPUStatusFlags lhs, CPUStatusFlags rhs) {
            using T = std::underlying_type_t <CPUStatusFlags>;
            return static_cast<CPUStatusFlags>(static_cast<T>(lhs) & static_cast<T>(rhs));
        }

        inline constexpr CPUStatusFlags operator ^ (CPUStatusFlags lhs, int rhs) {
            using T = std::underlying_type_t <CPUStatusFlags>;
            return static_cast<CPUStatusFlags>(static_cast<T>(lhs) ^ rhs);
        }


        inline constexpr CPUStatusFlags operator |= (CPUStatusFlags &lhs, CPUStatusFlags rhs) {
            lhs = lhs | rhs;
            return lhs;
        }

        inline constexpr CPUStatusFlags operator &= (CPUStatusFlags lhs, CPUStatusFlags rhs) {
            lhs = lhs & rhs;
            return lhs;
        }


        inline constexpr CPUStatusFlags operator << (bool bitvalue, CPUStatusFlagBitPos shiftvalue) {
            int res = (bitvalue?1:0) << static_cast<int>(shiftvalue);
            return static_cast<CPUStatusFlags>(res);
        }

        // Define some masks - not sure I actually use them...
        static constexpr CPUStatusFlags CPUStatusAritMask = CPUStatusFlags::Overflow | CPUStatusFlags::Carry | CPUStatusFlags::Zero | CPUStatusFlags::Negative | CPUStatusFlags::Extend;
        static constexpr CPUStatusFlags CPUStatusAritInvMask = (CPUStatusFlags::Overflow | CPUStatusFlags::Carry | CPUStatusFlags::Zero  | CPUStatusFlags::Negative | CPUStatusFlags::Extend) ^ 0xff;


        struct Registers {
            // 8 General purpose registers
            RegisterValue dataRegisters[8];
            // 7 + Stack - address registers
            RegisterValue addressRegisters[7];
            RegisterValue stackPointer;

            // Instruction pointer can't be modified
            RegisterValue instrPointer;
        };


        class InstructionDecoder;
        class CPUBase {
            friend InstructionDecoder;
        public:
            CPUBase() = default;
            virtual ~CPUBase() = default;


            const Registers &GetRegisters() const {
                return registers;
            }

            Registers &GetRegisters() {
                return registers;
            }
            const std::stack<RegisterValue> &GetStack() const {
                return stack;
            }

            const RegisterValue &GetInstrPtr() const {
                return registers.instrPointer;
            }

            RegisterValue &GetInstrPtr() {
                return registers.instrPointer;
            }

            std::stack<RegisterValue> &GetStack() {
                return stack;
            }


            const CPUStatusReg &GetStatusReg() const {
                return statusReg;
            }

            __inline const RegisterValue &GetRegisterValue(int idxRegister) const {
                return idxRegister>7?registers.addressRegisters[idxRegister-8]:registers.dataRegisters[idxRegister];
            }
            __inline RegisterValue &GetRegisterValue(int idxRegister) {
                return idxRegister>7?registers.addressRegisters[idxRegister-8]:registers.dataRegisters[idxRegister];
            }

            // Move this to base class
            RegisterValue ReadImmediateMode(OperandSize szOperand) {
                RegisterValue v = {};
                switch(szOperand) {
                    case OperandSize::Byte :
                        v.data.byte = FetchFromInstrPtr<uint8_t>();
                    break;
                    case OperandSize::Word :
                        v.data.word = FetchFromInstrPtr<uint16_t>();
                    break;
                    case OperandSize::DWord :
                        v.data.dword = FetchFromInstrPtr<uint32_t>();
                    break;
                    case OperandSize::Long :
                        v.data.longword = FetchFromInstrPtr<uint64_t>();
                    break;
                }
                return v;
            }

            // Move this to base class
            RegisterValue ReadFromMemory(OperandSize szOperand, uint64_t address) {
                RegisterValue v = {};
                switch(szOperand) {
                    case OperandSize::Byte :
                        v.data.byte = FetchFromRam<uint8_t>(address);
                    break;
                    case OperandSize::Word :
                        v.data.word = FetchFromRam<uint16_t>(address);
                    break;
                    case OperandSize::DWord :
                        v.data.dword = FetchFromRam<uint32_t>(address);
                    break;
                    case OperandSize::Long :
                        v.data.longword = FetchFromRam<uint64_t>(address);
                    break;
                }
                return v;
            }



        protected:
            template<typename T>
            T FetchFromInstrPtr() {
                auto address = registers.instrPointer.data.longword;
                T value = FetchFromRam<T>(address);
                registers.instrPointer.data.longword = address;
                return value;
/*
                if (address > szRam) {
                    fmt::println(stderr, "Invalid address {}", address);
                    exit(1);
                }
                T result = {};
                uint32_t nBits = 0;
                auto numToFetch = sizeof(T);
                while(numToFetch > 0) {
                    auto byte = ram[address];
                    address++;
                    // this results in 'hex' dumps easier to read - MSB first (most significant byte first) - Intel?
                    result = (result << nBits) | T(byte);
                    nBits = 8;
                    // this results in LSB - least significant byte first (Motorola?)
                    // result |= T(byte) << nBits);
                    // nBits += 8;
                    numToFetch -= 1;
                }
                registers.instrPointer.data.longword = address;
                return result;
*/
            }

            template<typename T>
            T FetchFromRam(uint64_t &address) {
                if (address > szRam) {
                    fmt::println(stderr, "Invalid address {}", address);
                    exit(1);
                }
                T result = {};
                uint32_t nBits = 0;
                auto numToFetch = sizeof(T);
                while(numToFetch > 0) {
                    // FIXME: delegate to 'memory-reader'
                    auto byte = ram[address];
                    address++;
                    // this results in 'hex' dumps easier to read - MSB first (most significant byte first) - Intel?
                    result = (result << nBits) | T(byte);
                    nBits = 8;
                    // this results in LSB - least significant byte first (Motorola?)
                    // result |= T(byte) << nBits);
                    // nBits += 8;
                    numToFetch -= 1;
                }
                return result;
            }



            // This one is used alot and should be faster...
            __inline uint8_t FetchByteFromInstrPtr() {
                return FetchFromInstrPtr<uint8_t>();
                /*
                            auto address = registers.instrPointer.data.longword;
                            auto byte = ram[address++];
                            registers.instrPointer.data.longword = address;
                            return byte;
                */
            }
            uint16_t FetchWordFromInstrPtr() {
                return FetchFromInstrPtr<uint16_t>();
            }
            uint32_t FetchDWordFromInstrPtr() {
                return FetchFromInstrPtr<uint32_t>();
            }
            uint64_t FetchLongFromInstrPtr() {
                return FetchFromInstrPtr<uint64_t>();
            }

        protected:
            uint8_t *ram = nullptr;
            size_t szRam = 0;
            Registers registers = {};
            CPUStatusReg statusReg = {};
            std::stack<RegisterValue> stack;


        };
    }
}



#endif //CPUBASE_H
