//
// Created by gnilk on 16.12.23.
//

#ifndef CPUBASE_H
#define CPUBASE_H

#include <stdint.h>
#include <type_traits>
#include <stack>
#include <functional>
#include <memory>
#include <mutex>

#include "fmt/format.h"
#include "MemorySubSys/MemoryUnit.h"
#include "Peripheral.h"
#include "Interrupt.h"
#include "RegisterValue.h"

#include "InstructionSet.h"

// Bring in some Peripherals
#include "Timer.h"

namespace gnilk {
    namespace vcpu {

        // perhaps copy M68k status bits...
        // FIXME: I would actually like to get rid of these - as it would make parallel execution so much simpler
        //        one idea is to have X number of ALU control registers and specific instructions using them
        //        and have the default instruction overflow/underflow predictably
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

        enum class CPUISRState {
            Waiting = 0,
            Flagged = 1,
            Executing = 2,
        };
        enum class CPUExceptionState {
            Idle = 0,
            Raised = 1,
            Executing = 2,
        };


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
            // Consider:
            //   - zero-register, fixed register with value 0 (makes 'bnz/bez' type of instructions easier, when/if we change the cmp/branch logic to 'bn<op> <regA>, <regB>, label'
            //
            // Layout:
            //  cr0 - INT Mask, zeroed out on reset (0 - disabled, 1 - enabled)     => gives 64 possible interrupts
            //  cr1 - Exception Mask, zeroed out on reset (0 - disabled, 1 - enabled) => gives 64 possible exceptions
            //  cr2 - Interrupt/Exception Status Register
            //  cr3 - mmu control register
            //  cr4 - mmu page table address
            //  cr5 - CPU ID or similar (feature register)
            //  cr6 - INT ID
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

            struct IntExceptionStatusControl {
                uint8_t interruptId : 8;        // 256 interrupts should be enough... whohahaha
                uint8_t intActive : 1;          // true if executing
                uint8_t intReserved : 7;        // <- could potentially be used as an extension bit

                uint8_t exceptionId : 8;        // 256 exception TYPES should be enough... whohahaha
                uint8_t expActive : 1;          // true if executing
                uint8_t expReserved : 7;        // <- coult potentially be used as an extension bit

                // Unused 32 bits in the control register...
                uint32_t future;
            };


            struct NamedControl {
                union {
                    RegisterValue intMaskReg = {};
                    CPUIntControl intControl;
                };
                union {
                    RegisterValue exceptionMaskReg = {};
                    CPUExceptionControl exceptionControl;
                };
                RegisterValue exceptionMask = {};
                union {
                    RegisterValue intExceptionStatusReg = {};
                    IntExceptionStatusControl intExceptionStatus;  // contains the active interrupt/exception register
                };
                RegisterValue mmuControl = {};
                RegisterValue mmuPageTableAddress = {};             // FIXME: could be moved to the 'MemoryLayout' block - no need to take a full register for this (or?)
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



        struct ISRControlBlock {
            CPUIntFlag intMask = {};
            CPUInterruptId interruptId = {};
            Registers registersBefore = {};
            RegisterValue rti = {};  // special register for RTI
            CPUISRState isrState = CPUISRState::Waiting;
        };

        struct ExceptionControlBlock {
            CPUExceptionFlag flag = {};
            Registers registersBefore = {};
            RegisterValue rte = {}; // This is RTI - reusing the same instructions
            CPUExceptionState state = CPUExceptionState::Idle;
        };

        //
        // This defines the memory layout in emulated RAM
        // Note: We can move various control registers here instead of allocating registers for them
        //
        struct MemoryLayout {
            ISR_VECTOR_TABLE    isrVectorTable = {};
            ExceptionControlBlock exceptionControlBlock  __attribute__ ((aligned (1024))) = {};
            ISRControlBlock isrControlBlocks[MAX_INTERRUPTS];

            // HW Mapping of various things - like GPIO, I2C, SPI, UART?

            // FIXME: Just testing a few things
            TimerConfigBlock timer0 = {};
            TimerConfigBlock timer1 = {};
            TimerConfigBlock timer2 = {};
            TimerConfigBlock timer3 = {};
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

        // CPUBase is more of a 'Core'
        // Not sure if I should compose this with a 'Core' class that holds
        // - CPU emulation (instr. decoding)
        // - MMU
        // The 'CORE' would then be placed on the SOC level...
        // SOC holds the DataBus and so forth...


        class InstructionDecoderBase;
        // FIXME: the ISR controller should be part of the SOC
        class CPUBase : public InterruptController {
            friend InstructionDecoderBase;
        public:
            using Ref = std::shared_ptr<CPUBase>;
        public:
            CPUBase() = default;
            virtual ~CPUBase();

            virtual void QuickStart(void *ptrRam, size_t sizeOfRam);
            virtual void Begin(void *ptrRam, size_t sizeOfRam);
            virtual void End();
            virtual void Reset();

            bool RegisterSysCall(uint16_t id, const std::string &name, SysCallDelegate handler);

            bool IsHalted() const {
                return registers.statusReg.flags.halt;
            }

            void Halt() {
                registers.statusReg.flags.halt = 1;
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

            MemoryLayout *GetSystemMemoryBlock();

            const CPUStatusReg &GetStatusReg() const {
                return registers.statusReg;
            }

            CPUIntControl &GetInterruptCntrl() {
                return registers.cntrlRegisters.named.intControl;
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
            void EnableInterrupt(CPUIntFlag interrupt);
            bool AddPeripheral(CPUIntFlag intMAsk, CPUInterruptId interruptId, Peripheral::Ref peripheral);
            void DelPeripherals();
            void ResetPeripherals();
            virtual void UpdatePeripherals();

            // Interrupt Controller Interface
            bool IsCPUISRActive();
            bool IsCPUExpActive();

            void RaiseInterrupt(CPUInterruptId interruptId) override;
            // Returns
            //   true if any of the ISR handlers changed the current Instr.Ptr in order to execute next
            bool InvokeISRHandlers();

            CPUISRState GetISRState(CPUInterruptId interruptId) {
                return GetISRControlBlock(interruptId).isrState;
            }

            ISRControlBlock &GetISRControlBlock(CPUInterruptId interruptId) {
                int blockIndex = interruptId & (MAX_INTERRUPTS - 1);
                return isrControlBlocks[blockIndex];
            }

            // Exceptions
            void EnableException(CPUExceptionId  exceptionId);
            virtual bool RaiseException(CPUExceptionId exceptionId);
            bool InvokeExceptionHandlers(CPUExceptionId exceptionId);

            // FIXME: Solve this - these are public since instruction set's inherits and friend's can't follow inheritance
        public:
            void DoEnd();
            ISRControlBlock *GetActiveISRControlBlock();
            void ResetActiveISR();
            void SetActiveISR(CPUInterruptId interruptId);

            void SetCPUISRActiveState(bool isActive);
            void SetCPUExpActiveState(bool isActive);
            void SetActiveException(CPUExceptionId exceptionId);
            void ResetActiveExp();

            bool IsExceptionEnabled(CPUExceptionId  exceptionFlag);

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
       // FIXME: Solve this - these are public since instruction set's inherits and friend's can't follow inheritance
        public:
            struct ISRPeripheralInstance {
                CPUIntFlag intMask;
                CPUInterruptId interruptId;
                Peripheral::Ref peripheral;
            };
        // FIXME: Solve this - these are public since instruction set's inherits and friend's can't follow inheritance
        public:
            uint8_t *ram = nullptr;
            size_t szRam = 0;
            Registers registers = {};


            //MemoryUnit memoryUnit;
            MMU memoryUnit;
            MemoryLayout *systemBlock = nullptr;

            // Short cut pointers into the systemBlock...
            ISR_VECTOR_TABLE *isrVectorTable = nullptr;
            ISRControlBlock *isrControlBlocks = nullptr;
            ExceptionControlBlock *expControlBlock = nullptr;
            // End short cut pointers


            std::mutex isrLock;

            std::vector<ISRPeripheralInstance> peripherals;

            // FIXME: Remove this and let the supplied RAM hold the stack...
            std::stack<RegisterValue> stack;

            std::unordered_map<CPUInterruptId , CPUIntFlag> interruptMapping;
            std::unordered_map<uint32_t, SysCall::Ref> syscalls;
        };
    }
}



#endif //CPUBASE_H
