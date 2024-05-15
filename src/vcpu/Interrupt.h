//
// Created by gnilk on 15.01.2024.
//

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include<stdint.h>
#include <memory>

namespace gnilk {
    namespace vcpu {
        // These are part of the CPU emulation and NOT callbacks to the emulator...
        typedef uint64_t ISR_FUNC;
        typedef uint64_t EXP_FUNC;

        // MUST BE POWER of 2, used like this: index = interrupt & (MAX_INTERRUPTS - 1);
        // Maximum we can hold is currently 256 interrupts
        // this is defined by the CR2 (Control Register 2) layout. Which reserves 8 bits for the interrupt ID..
        static constexpr int MAX_INTERRUPTS = 8;

        // All except initial_sp/pc are set to 0 during startup
        // initial_sp = last byte in RAM
        // initial_pc = VCPU_INITIAL_PC (which is default to 0x2000)
#pragma pack(push,1)
        struct ISR_VECTOR_TABLE {
            uint64_t initial_sp;        // 0
            uint64_t initial_pc;        // 1
            // Exceptions
            // FIXME: we can detect many-more, like
            //  - invalid-isr state (issuing RTI outside of an interrupt)
            //  - invalid-exp state (issuing RTE outside of an exception handler)
            //  - invalid-address mode (or unsupported address mode)
            //  - stack-empty (any auto-stack decrement function where the stack is already empty, pop/ret)
            //
            EXP_FUNC exp_illegal_instr; // 2
            EXP_FUNC exp_invalid_addrmode; // 2
            EXP_FUNC exp_hard_fault;    // 3        // Or reset vector...
            EXP_FUNC exp_div_zero;      // 4
            EXP_FUNC exp_debug_trap;    // 5
            EXP_FUNC exp_mmu_fault;     // 6
            EXP_FUNC exp_fpu_fault;     // 7

            // Interrupt Service Routines
            ISR_FUNC isr0;        // 8
            ISR_FUNC isr1;        // 9
            ISR_FUNC isr2;        // 10
            ISR_FUNC isr3;        // 11
            ISR_FUNC isr4;        // 12
            ISR_FUNC isr5;        // 13
            ISR_FUNC isr6;        // 14
            ISR_FUNC isr7;        // 15
            uint64_t reserved[16];
        };
#pragma pack(pop)

        // This should go into the CPUIntCntrlRegister as a mask...
        struct CPUExceptionControlBits {
            // CPU hard interrupts
            uint16_t hard_fault:1;          // 0
            uint16_t illegal_instr:1;       // 1
            uint16_t illegal_addr_mode:1;   // 2
            uint16_t div_zero:1;            // 3
            uint16_t debug_trap:1;          // 4
            uint16_t mmu_fault:1;           // 5
            uint16_t fpu_fault:1;           // 6
            uint16_t cpu_reserved_1:1;      // 7

            // Let's reserve another 8 so we have 16 bits in total for this...
            uint16_t reserved:8;
        };

        struct CPUExceptionControl {
            union {
                CPUExceptionControlBits flags;
                uint16_t bits;
            } data;
        };

        typedef uint64_t CPUExceptionId;

        typedef enum : CPUExceptionId {
            kHardFault = 0,
            kInvalidInstruction = 1,
            kInvalidAddrMode = 2,
            kDivisionByZero = 3,
            kDebugTrap = 4,
            kMMUFault = 5,
            kFPUFault = 6,
        } CPUKnownExceptions;


        typedef enum : uint64_t {
            HardFault = 0x01,
            InvalidInstruction = 0x02,
            InvalidAddrMode = 0x04,
            DivisionByZero = 0x08,
            DebugTrap = 0x10,
            MMUFault = 0x20,
            FPUFault = 0x40,
        }  CPUExceptionFlag;




        struct CPUIntControlBits {
            uint16_t int0 : 1;
            uint16_t int1 : 1;
            uint16_t int2 : 1;
            uint16_t int3 : 1;
            uint16_t int4 : 1;
            uint16_t int5 : 1;
            uint16_t int6 : 1;
            uint16_t int7 : 1;
            uint16_t reserved : 8;
        };
        struct CPUIntControl {
            union {
                CPUIntControlBits flags;
                uint16_t bits;
            } data;
        };

        enum CPUIntFlag : uint16_t {
            INT0 = 1,
            INT1 = 2,
            INT2 = 4,
            INT3 = 8,
            INT4 = 16,
            INT5 = 32,
            INT6 = 64,
            INT7 = 128,
        };

        inline constexpr bool operator & (CPUIntControl lhs, CPUIntFlag rhs) {
            return lhs.data.bits & rhs;
        }

        typedef uint8_t CPUInterruptId;

        // Not quite sure I want/need this
        enum CPUKnownIntIds : uint64_t {
            kTimer0,
        };

        class InterruptController {
        public:
            InterruptController() = default;
            virtual ~InterruptController() = default;

            virtual void RaiseInterrupt(CPUInterruptId isrType) = 0;
        };
    }
}

#endif //INTERRUPT_H
