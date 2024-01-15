//
// Created by gnilk on 16.12.23.
//

#ifndef CPUBASE_H
#define CPUBASE_H

#include <stdint.h>
#include <type_traits>
#include <stack>
#include <functional>

#include "fmt/format.h"
#include "InstructionSet.h"
#include "MemoryUnit.h"

namespace gnilk {
    namespace vcpu {

        // These are part of the CPU emulation and NOT callbacks to the emulator...
        typedef uint64_t ISR_FUNC;
        // All except initial_sp/pc are set to 0 during startup
        // initial_sp = last byte in RAM
        // initial_pc = VCPU_INITIAL_PC (which is default to 0x2000)
#pragma pack(push,1)
        struct ISR_VECTOR_TABLE {
            uint64_t initial_sp;        // 0
            uint64_t initial_pc;        // 1
            ISR_FUNC isr_illegal_instr; // 2
            ISR_FUNC isr_hard_fault;    // 3
            ISR_FUNC isr_div_zero;      // 4
            ISR_FUNC isr_debug_trap;    // 5
            ISR_FUNC isr_mmu_fault;     // 6
            ISR_FUNC isr_fpu_fault;     // 7
            ISR_FUNC isr_ext_l1;        // 8
            ISR_FUNC isr_ext_l2;        // 9
            ISR_FUNC isr_ext_l3;        // 10
            ISR_FUNC isr_ext_l4;        // 11
            ISR_FUNC isr_ext_l5;        // 12
            ISR_FUNC isr_ext_l6;        // 13
            ISR_FUNC isr_ext_l7;        // 14
            ISR_FUNC isr_ext_l8;        // 15
            uint64_t reserved[16];
        };
#pragma pack(pop)

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
            uint16_t carry : 1;
            uint16_t overflow : 1;
            uint16_t zero : 1;
            uint16_t negative : 1;
            uint16_t extend : 1;
            uint16_t int1 : 1;
            uint16_t int2 : 1;
            uint16_t int3 : 1;
            // next 8 bits
            uint16_t halt : 1;
            uint16_t reserved : 7;
        };

        enum class CPUStatusFlags : uint16_t {
            None = 0,
            Carry = 1,
            Overflow = 2,
            Zero = 4,
            Negative = 8,
            Extend = 16,
            Int1 = 32,
            Int2 = 64,
            Int3 = 128,
            Halt = 256,
        };

        enum class CPUStatusFlagBitPos : uint16_t {
            Carry = 0,
            Overflow = 1,
            Zero = 2,
            Negative = 3,
            Extend = 4,
            Int1 = 5,
            Int2 = 6,
            Int3 = 7,
            Halt = 8,
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

            // Control registers - if the instr. has the 'OperandFamily == Control' set you can access these...
            // cr0 - mmu control register
            // cr1 - mmu page table address
            RegisterValue cntrlRegisters[8];

            // Instruction pointer can't be modified
            RegisterValue instrPointer;
        };
        struct Control {
        };


        static const uint64_t VCPU_RESERVED_RAM = 0x2000;
        static const uint64_t VCPU_INITIAL_PC = 0x2000;

        class CPUBase;

        using SysCallDelegate = std::function<void(Registers &regs, CPUBase *cpu)>;

        class SysCall {
        public:
            using Ref = std::shared_ptr<SysCall>;   // this can probably be unique ptr
        public:
            SysCall(uint16_t sysId, const std::string &sysName, SysCallDelegate handler) : id(sysId), name(sysName), cbHandler(handler) {

            }
            virtual ~SysCall() = default;
            static SysCall::Ref Create(uint16_t id, const std::string &name, SysCallDelegate handler) {
                return std::make_shared<SysCall>(id, name, handler);
            }
            void Invoke(Registers &regs, CPUBase *cpu) {
                cbHandler(regs, cpu);
            }
        protected:
            uint16_t id;
            std::string name;
            SysCallDelegate cbHandler;
        };


        class InstructionDecoder;
        class CPUBase {
            friend InstructionDecoder;
        public:
            CPUBase() = default;
            virtual ~CPUBase() = default;

            virtual void QuickStart(void *ptrRam, size_t sizeOfRam);

            virtual void Begin(void *ptrRam, size_t sizeOfRam);
            virtual void Reset();
            bool RegisterSysCall(uint16_t id, const std::string &name, SysCallDelegate handler);

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

            void SetInstrPtr(uint64_t newIp) {
                registers.instrPointer.data.longword = newIp;
            }

            void AdvanceInstrPtr(uint64_t ipOffset) {
                registers.instrPointer.data.longword += ipOffset;
            }
            RegisterValue &GetInstrPtr() {
                return registers.instrPointer;
            }

            std::stack<RegisterValue> &GetStack() {
                return stack;
            }

            void *GetRawPtrToRAM(uint64_t addr);

            const CPUStatusReg &GetStatusReg() const {
                return statusReg;
            }
            uint8_t *GetRamPtr() {
                return ram;
            }
            size_t GetRamSize() {
                return szRam;
            }
            bool LoadDataToRam(uint64_t ramAddress, const void *ptrData, size_t szData) {
                if ((ramAddress + szData) > szRam) {
                    return false;
                }
                memcpy(&ram[ramAddress], ptrData, szData);
                return true;
            }

            __inline const RegisterValue &GetRegisterValue(int idxRegister, OperandFamily family) const {
                if (family == OperandFamily::Control) {
                    return idxRegister>7?registers.cntrlRegisters[idxRegister-8]:registers.dataRegisters[idxRegister];
                }
                return idxRegister>7?registers.addressRegisters[idxRegister-8]:registers.dataRegisters[idxRegister];
            }
            __inline RegisterValue &GetRegisterValue(int idxRegister, OperandFamily family) {
                if (family == OperandFamily::Control) {
                    return idxRegister>7?registers.cntrlRegisters[idxRegister-8]:registers.dataRegisters[idxRegister];
                }
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

            // Read with address translation
            RegisterValue ReadFromMemoryUnit(OperandSize szOperand, uint64_t address) {
                RegisterValue v = {};

                address = memoryUnit.TranslateAddress(address);

                switch(szOperand) {
                    case OperandSize::Byte :
                        v.data.byte = FetchFromPhysicalRam<uint8_t>(address);
                    break;
                    case OperandSize::Word :
                        v.data.word = FetchFromPhysicalRam<uint16_t>(address);
                    break;
                    case OperandSize::DWord :
                        v.data.dword = FetchFromPhysicalRam<uint32_t>(address);
                    break;
                    case OperandSize::Long :
                        v.data.longword = FetchFromPhysicalRam<uint64_t>(address);
                    break;
                }
                return v;
            }

            virtual void UpdatePeripherals();




        protected:
            template<typename T>
            T FetchFromInstrPtr() {
                auto address = registers.instrPointer.data.longword;
                T value = FetchFromPhysicalRam<T>(address);
                registers.instrPointer.data.longword = address;
                return value;
            }

            // Read from physical memory..
            template<typename T>
            T FetchFromPhysicalRam(uint64_t &address) {
                if (address > szRam) {
                    fmt::println(stderr, "FetchFromRam - Invalid address {}", address);
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
            void UpdateMMU();
        protected:
            uint8_t *ram = nullptr;
            size_t szRam = 0;
            Registers registers = {};
            CPUStatusReg statusReg = {};
            MemoryUnit memoryUnit;

            ISR_VECTOR_TABLE *isrVectorTable = nullptr;

            // FIXME: Remove this and let the supplied RAM hold the stack...
            std::stack<RegisterValue> stack;

            std::unordered_map<uint32_t, SysCall::Ref> syscalls;


        };
    }
}



#endif //CPUBASE_H
