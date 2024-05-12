//
// Created by gnilk on 16.12.23.
//

#include "fmt/format.h"
#include "CPUBase.h"


using namespace gnilk;
using namespace gnilk::vcpu;

// QuickStart won't initialize the ISR Table and reserve stuff - it will just assign the RAM ptr
void CPUBase::QuickStart(void* ptrRam, size_t sizeOfRam) {
    ram = static_cast<uint8_t *>(ptrRam);
    szRam = sizeOfRam;

    memset(&registers, 0, sizeof(registers));
}

void CPUBase::Begin(void* ptrRam, size_t sizeOfRam) {
    ram = static_cast<uint8_t *>(ptrRam);
    szRam = sizeOfRam;
    Reset();
}

void CPUBase::Reset() {
    // Everything is zero upon reset...
    memset(&registers, 0, sizeof(registers));
    memset(ram, 0, szRam);

    isrVectorTable = (ISR_VECTOR_TABLE*)ram;

    isrVectorTable->initial_sp = szRam-1;
    isrVectorTable->initial_pc = VCPU_INITIAL_PC;

}



void *CPUBase::GetRawPtrToRAM(uint64_t addr) {
    if (addr > szRam) {
        return nullptr;
    }
    return &ram[addr];
}

bool CPUBase::RegisterSysCall(uint16_t id, const std::string &name, SysCallDelegate handler) {
    if (syscalls.contains(id)) {
        fmt::println(stderr, "SysCall with id {} ({:#x}) already exists",  id,id);
        return false;
    }
    auto inst = SysCall::Create(id, name, handler);
    syscalls[id] = inst;
    return true;
}
void CPUBase::UpdateMMU() {
    // FIXME: refactor mmu
    auto mmuControl0 = registers.cntrlRegisters.named.mmuControl;
    auto mmuControl1 = registers.cntrlRegisters.named.mmuPageTableAddress;
//    memoryUnit.SetControl(cr0);
//    memoryUnit.SetPageTranslationVAddr(cr1);
}

void CPUBase::AddPeripheral(CPUIntMask intMask, CPUInterruptId interruptId, Peripheral::Ref peripheral) {
    ISRPeripheralInstance instance = {
        .intMask = intMask,
        .interruptId = interruptId,
        .peripheral =  peripheral
    };
    // TMP TMP TMP
    interruptMapping[interruptId] = intMask;

    peripheral->SetInterruptController(this);
    peripheral->MapToInterrupt(interruptId);
    peripherals.push_back(instance);
}

void CPUBase::ResetPeripherals() {
    for(auto &p : peripherals) {
        p.peripheral->Initialize();
    }
}

void CPUBase::UpdatePeripherals() {
    for(auto &p : peripherals) {
        p.peripheral->Update();
    }
}

void CPUBase::RaiseInterrupt(CPUInterruptId interruptId) {
    if (isrControlBlock.isrState != CPUISRState::Waiting) {
        // Already within an ISR - do NOT execute another
        return;
    }

    // Is this mapped??
    if (interruptMapping.find(interruptId) == interruptMapping.end()) {
        return;
    }

    auto mask = interruptMapping[interruptId];
    auto &intCntrl = GetInterruptCntrl();

    // Is this enabled?
    if (!(intCntrl.data.longword & mask)) {
        return;
    }

    //
    // Need to queue interrupts - as the CPU status register can only hold 1 ISR combo at any given time
    //
    // the TYPE is mapped to an interrupt level (there are 8) - in total we should be able to handle X ISR handlers
    // with TYPE <-> INT mappings...
    //
    isrControlBlock.intMask = mask;
    isrControlBlock.interruptId = interruptId;
    isrControlBlock.isrState = CPUISRState::Flagged;
}

void CPUBase::EnableInterrupt(CPUIntMask interrupt) {
    auto &intCntrl = GetInterruptCntrl();
    intCntrl.data.longword |= interrupt;
}

void CPUBase::InvokeISRHandlers() {
    if (isrVectorTable == nullptr) {
        return;
    }
    auto &intCntrl = GetInterruptCntrl();
    for(int i=0;i<7;i++) {
        // Is this enabled and raised?
        if ((l64a(intCntrl.data.longword & (1<<i))) && (isrControlBlock.isrState == CPUISRState::Flagged)) {
            // Save current registers
            isrControlBlock.registersBefore = registers;
            // Move the ISR type to a register...
            registers.dataRegisters[0].data.longword = isrControlBlock.interruptId;
            // reassign it..
            registers.instrPointer.data.longword = isrVectorTable->isr0;
            // Update the state
            isrControlBlock.isrState = CPUISRState::Executing;
        }
    }
}

//
// Exceptions
// Similar to interrupts except;
// - raised directly, CPU instr.ptr is changed and next stepping will be in the interrupt handler
// - in case of exception within an exception handler, we will halt the CPU
//
bool CPUBase::RaiseException(CPUExceptionId exceptionId) {
    fmt::println(stderr, "CPUBase, Exception - {}", (int)exceptionId);

    // Don't raise exceptions within the exception handler
    if (expControlBlock.isrState != CPUISRState::Waiting) {
        // Already within an exception
        // FIXME: Reboot..
        return false;
    }

    auto &exceptionCntrl = GetExceptionCntrl();

    // Is this enabled?
    if (!(exceptionCntrl.data.longword & exceptionId)) {
        return false;
    }


    expControlBlock.intMask = {};
    expControlBlock.interruptId = 0;
    expControlBlock.isrState = CPUISRState::Flagged;

    return InvokeExceptionHandlers(exceptionId);
}
void CPUBase::EnableException(int exceptionMask) {
    auto &exceptionControl = GetExceptionCntrl();
    exceptionControl.data.longword |= exceptionMask;
}

bool CPUBase::InvokeExceptionHandlers(CPUExceptionId exceptionId)  {
    if (isrVectorTable == nullptr) {
        return false;
    }
    auto &exceptionControl = GetExceptionCntrl();
    for(int i=0;i<7;i++) {
        // Is this enabled and raised?
        if (exceptionControl.data.longword & (1<<i)) {
            // Save current registers
            expControlBlock.registersBefore = registers;
            // Move the ISR type to a register...
            registers.dataRegisters[0].data.longword = exceptionId;
            // Note: take this depending on the exception type...
            registers.instrPointer.data.longword = isrVectorTable->exp_illegal_instr;
            // Update the state
            expControlBlock.isrState = CPUISRState::Executing;

            return true;
        }
    }
    return false;
}
