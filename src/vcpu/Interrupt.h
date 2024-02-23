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
            // FIXME: Verify this - perhaps it is easier to have specific interrupts than just 'l1,l2,l3,l4'...
            //        Otherwise I would need a muxing table (timer0 -> ext_l1, keyboard -> ext_l2, etc...)
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

        // This should go into the CPUIntCntrlRegister as a mask...
        struct CPUIntMaskBits {
            // CPU hard interrupts
            uint16_t illegal_instr:1;   // 0
            uint16_t hard_fault:1;      // 1
            uint16_t div_zero:1;        // 2
            uint16_t debug_trap:1;      // 3
            uint16_t mmu_fault:1;       // 4
            uint16_t fpu_fault:1;       // 5
            uint16_t cpu_reserved_1:1;  // 6
            uint16_t cpu_reserved_2:1;  // 7

            // Peripherals, 8 possible interrupts
            uint16_t ext_l1:1;
            uint16_t ext_l2:1;
            uint16_t ext_l3:1;
            uint16_t ext_l4:1;
            uint16_t ext_l5:1;
            uint16_t ext_l6:1;
            uint16_t ext_l7:1;
            uint16_t ext_l8:1;
        };

        enum class CPUISRType : uint8_t {
            None,
            Timer0,
        };
        class InterruptController {
        public:
            InterruptController() = default;
            virtual ~InterruptController() = default;

            virtual void RaiseInterrupt(CPUISRType isrType) = 0;
        };
    }
}

#endif //INTERRUPT_H
