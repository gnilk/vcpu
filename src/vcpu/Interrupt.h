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
