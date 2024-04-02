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
#include "MemorySubSys/MemoryUnit.h"
#include "Peripheral.h"
#include "Interrupt.h"
#include "RegisterValue.h"

namespace gnilk {
    namespace vcpu {

        // perhaps copy M68k status bits...
        struct CPUStatusBits {
            uint16_t carry : 1;
            uint16_t overflow : 1;
            uint16_t zero : 1;
            uint16_t negative : 1;
            uint16_t extend : 1;
            uint16_t halt : 1;
            uint16_t reservedA : 1;      // old int flags - currently unused
            uint16_t reservedB : 1;
            uint16_t reservedC : 1;
            // next 8 bits
            uint16_t reserved : 7;
        };

        enum class CPUStatusFlags : uint16_t {
            None = 0,
            Carry = 1,
            Overflow = 2,
            Zero = 4,
            Negative = 8,
            Extend = 16,
            Halt = 32,
        };

        enum class CPUStatusFlagBitPos : uint16_t {
            Carry = 0,
            Overflow = 1,
            Zero = 2,
            Negative = 3,
            Extend = 4,
            Halt = 5,
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
            // FIXME: decide how these should be laid out
            // we need
            //   - mmu control register
            //   - mmu page table address
            //   - CPU Interrupt mask register
            //   - Exception mask register
            //   - Copy of CPU Status Register?
            //   - Other?
            //
            // Layout:
            //  cr0 - INT Mask, zeroed out on reset (0 - disabled, 1 - enabled)     => gives 64 possible interrupts
            //  cr1 - Exception Mask, zeroed out on reset (0 - disabled, 1 - enabled) => gives 64 possible exceptions
            //  cr2 - Status Register copy (is this necessary - I think, it makes it possible to copy the whole status reg over to something else)
            //  cr3 - mmu control register
            //  cr4 - mmu page table address
            //  cr5 - CPU ID or similar (feature register)
            //  cr6 - reserved, unused
            //  cr7 - reserved, unused
            struct Control {
                RegisterValue cr0 = {};
                RegisterValue cr1 = {};
                RegisterValue cr2 = {};
                RegisterValue cr4 = {};
                RegisterValue cr5 = {};
                RegisterValue cr6 = {};
                RegisterValue cr7 = {};
            };
            struct NamedControl {
                RegisterValue intMask = {};
                RegisterValue exceptionMask = {};
                RegisterValue statusRegCopy = {};
                RegisterValue mmuControl = {};
                RegisterValue mmuPageTableAddress = {};
                RegisterValue cpuid = {};
                RegisterValue reservedA = {};
                RegisterValue reservedB = {};
            };

            union CntrlRegisters {
                Control regs;
                NamedControl named;
                RegisterValue array[8];
            };
            //RegisterValue cntrlRegisters[8];
            CntrlRegisters cntrlRegisters = {};

            // Instruction pointer can't be modified
            RegisterValue instrPointer;

            CPUStatusReg statusReg = {};
        };

        enum class CPUISRState {
            Waiting = 0,
            Flagged = 1,
            Executing = 2,
        };


        struct ISRControlBlock {
            CPUIntMask intMask = {};
            CPUInterruptId interruptId = {};
            Registers registersBefore = {};
            RegisterValue rti = {};  // special register for RTI
            CPUISRState isrState = CPUISRState::Waiting;
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
        class CPUBase : public InterruptController {
            friend InstructionDecoder;
        public:
            CPUBase() = default;
            virtual ~CPUBase() = default;

            virtual void QuickStart(void *ptrRam, size_t sizeOfRam);

            virtual void Begin(void *ptrRam, size_t sizeOfRam);
            virtual void Reset();
            bool RegisterSysCall(uint16_t id, const std::string &name, SysCallDelegate handler);

            bool IsHalted() const {
                return registers.statusReg.flags.halt;
            }

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
                return registers.statusReg;
            }

            RegisterValue &GetInterruptCntrl() {
                return registers.cntrlRegisters.named.intMask;
            }

            RegisterValue &GetExceptionCntrl() {
                return registers.cntrlRegisters.named.exceptionMask;
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
                    return idxRegister>7?registers.cntrlRegisters.array[idxRegister-8]:registers.dataRegisters[idxRegister];
                }
                return idxRegister>7?registers.addressRegisters[idxRegister-8]:registers.dataRegisters[idxRegister];
            }
            __inline RegisterValue &GetRegisterValue(int idxRegister, OperandFamily family) {
                if (family == OperandFamily::Control) {
                    return idxRegister>7?registers.cntrlRegisters.array[idxRegister-8]:registers.dataRegisters[idxRegister];
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

            void WriteToMemoryUnit(OperandSize szOperand, uint64_t address, RegisterValue value) {
                address = memoryUnit.TranslateAddress(address);
                switch(szOperand) {
                    case OperandSize::Byte :
                        WriteToPhysicalRam<uint8_t>(address, value.data.byte);
                        break;
                    case OperandSize::Word :
                        WriteToPhysicalRam<uint16_t>(address, value.data.word);
                    break;
                    case OperandSize::DWord :
                        WriteToPhysicalRam<uint32_t>(address, value.data.dword);
                    break;
                    case OperandSize::Long :
                        WriteToPhysicalRam<uint64_t>(address, value.data.longword);
                    break;

                }

            }
            void EnableInterrupt(CPUIntMask interrupt);
            void AddPeripheral(CPUIntMask intMAsk, CPUInterruptId interruptId, Peripheral::Ref peripheral);
            void ResetPeripherals();
            virtual void UpdatePeripherals();
        // Interrupt Controller Interface
            void RaiseInterrupt(CPUInterruptId interruptId) override;
            void InvokeISRHandlers();
            CPUISRState GetISRState() {
                return isrControlBlock.isrState;
            }

        protected:
            template<typename T>
            T FetchFromInstrPtr() {
                auto address = registers.instrPointer.data.longword;
                T value = FetchFromPhysicalRam<T>(address);
                registers.instrPointer.data.longword = address;
                return value;
            }

            template<typename T>
            void WriteToPhysicalRam(uint64_t &address, const T &value) {
                if ((address + sizeof(T)) > szRam) {
                    fmt::println(stderr, "WriteToRam - Invalid address {} exceeds physical memory", address);
                    exit(1);
                }
                auto numToWrite = sizeof(T);
                auto bitShift = (numToWrite-1)<<3;
                while(numToWrite > 0) {

                    ram[address] = (value >> bitShift) & 0xff;

                    bitShift -= 8;
                    address++;
                    numToWrite -= 1;
                }
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
            struct ISRPeripheralInstance {
                CPUIntMask intMask;
                CPUInterruptId interruptId;
                Peripheral::Ref peripheral;
            };
        protected:
            uint8_t *ram = nullptr;
            size_t szRam = 0;
            Registers registers = {};

            // Used to stash all information during an interrupt..
            // Note: We should have one of these per interrupt - 8 of them...
            ISRControlBlock isrControlBlock;

            MemoryUnit memoryUnit;

            ISR_VECTOR_TABLE *isrVectorTable = nullptr;

            std::vector<ISRPeripheralInstance> peripherals;

            // FIXME: Remove this and let the supplied RAM hold the stack...
            std::stack<RegisterValue> stack;

            std::unordered_map<CPUInterruptId , CPUIntMask> interruptMapping;
            std::unordered_map<uint32_t, SysCall::Ref> syscalls;


        };
    }
}



#endif //CPUBASE_H
