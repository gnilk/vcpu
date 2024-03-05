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
    auto mmuControl0 = registers.cntrlRegisters[0].data.mmuControl0;
    auto mmuControl1 = registers.cntrlRegisters[1].data.mmuControl1;
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
    if (!(intCntrl & mask)) {
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
    intCntrl.data.bits |= interrupt;
}

void CPUBase::InvokeISRHandlers() {
    if (isrVectorTable == nullptr) {
        return;
    }
    auto &intCntrl = GetInterruptCntrl();
    for(int i=0;i<7;i++) {
        // Is this enabled and raised?
        if ((intCntrl.data.bits & (1<<i)) && (isrControlBlock.isrState == CPUISRState::Flagged)) {
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
