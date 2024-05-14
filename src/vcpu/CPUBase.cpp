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
    if (GetISRState(interruptId) != CPUISRState::Waiting) {
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
    auto &isrControlBlock = GetISRControlBlock(interruptId);
    isrControlBlock.intMask = mask;
    isrControlBlock.interruptId = interruptId;
    isrControlBlock.isrState = CPUISRState::Flagged;
}

void CPUBase::EnableInterrupt(CPUIntMask interrupt) {
    auto &intCntrl = GetInterruptCntrl();
    intCntrl.data.longword |= interrupt;
}

ISRControlBlock *CPUBase::GetActiveISRControlBlock() {
    auto interruptId = registers.cntrlRegisters.named.intExceptionStatus.interruptId;
    return &isrControlBlocks[interruptId];
}

// This will update the Interrupt/Exception CR status register
void CPUBase::SetCPUISRActiveState(bool isActive) {
    registers.cntrlRegisters.named.intExceptionStatus.intActive = isActive?1:0;
}
bool CPUBase::IsCPUISRActive() {
    return (registers.cntrlRegisters.named.intExceptionStatus.intActive==1);
}

void CPUBase::ResetActiveISR() {

    // Reset the control block for the active ISR..
    auto isrControlBlock = GetActiveISRControlBlock();
    isrControlBlock->isrState = CPUISRState::Waiting;

    // Reset status register values
    SetCPUISRActiveState(false);
    registers.cntrlRegisters.named.intExceptionStatus.interruptId = 0;
}

void CPUBase::SetActiveISR(CPUInterruptId interruptId) {
    registers.cntrlRegisters.named.intExceptionStatus.interruptId = interruptId;
    SetCPUISRActiveState(true);
}

bool CPUBase::InvokeISRHandlers() {
    if (isrVectorTable == nullptr) {
        return false;
    }
    auto &intCntrl = GetInterruptCntrl();
    for(int i=0;i<MAX_INTERRUPTS;i++) {
        // Is this enabled and raised?
        auto &isrControlBlock = GetISRControlBlock(i);

        if ((intCntrl.data.longword & (1<<i)) && (isrControlBlock.isrState == CPUISRState::Flagged)) {
            // Save current registers
            isrControlBlock.registersBefore = registers;
            // Move the ISR type to a register...
            registers.dataRegisters[0].data.longword = isrControlBlock.interruptId;

            // reassign it..
            registers.instrPointer.data.longword = isrVectorTable->isr0;
            // Update the state
            isrControlBlock.isrState = CPUISRState::Executing;

            // Set it active
            SetActiveISR(isrControlBlock.interruptId);

            // We need to break now, since we will be executing an ISR - and even if there is another pending - we can't have multiple...
            return true;
        }
    }
    return false;
}

//
// Exceptions
// Similar to interrupts except;
// - raised directly, CPU instr.ptr is changed and next stepping will be in the interrupt handler
// - in case of exception within an exception handler, we will halt the CPU
//
bool CPUBase::RaiseException(CPUExceptionFlag exception) {

    if (IsCPUExpActive()) {
        fmt::println(stderr, "CPUBase, nested exception detected, halting CPU");
        Halt();
        return false;
    }

    // Is it enabled?
    if (!IsExceptionEnabled(exception)) {
        fmt::println(stderr, "CPUBase, exception not enabled for {:#x} - halting cpu", (int)exception);
        Halt();
        return false;
    }

    fmt::println(stderr, "CPUBase, raising exception - {:#x}", (int)exception);

    expControlBlock.flag = exception;
    expControlBlock.state = CPUExceptionState::Raised;

    return InvokeExceptionHandlers(exception);
}

bool CPUBase::IsExceptionEnabled(CPUExceptionFlag  exceptionFlag) {
    auto &exceptionControl = GetExceptionCntrl();
    return (exceptionControl.data.longword & exceptionFlag);
}

void CPUBase::EnableException(CPUExceptionFlag exception) {
    auto &exceptionControl = GetExceptionCntrl();
    exceptionControl.data.longword |= exception;
}

void CPUBase::SetActiveException(CPUExceptionFlag exceptionId) {
    registers.cntrlRegisters.named.intExceptionStatus.exceptionId = exceptionId;
    SetCPUExpActiveState(true);
}

void CPUBase::SetCPUExpActiveState(bool isActive) {
    registers.cntrlRegisters.named.intExceptionStatus.expActive = isActive?1:0;
}

bool CPUBase::IsCPUExpActive() {
    return (registers.cntrlRegisters.named.intExceptionStatus.expActive == 1);
}

void CPUBase::ResetActiveExp() {
    SetCPUExpActiveState(false);
    expControlBlock.state = CPUExceptionState::Idle;
}

bool CPUBase::InvokeExceptionHandlers(CPUExceptionFlag exception)  {
    if (isrVectorTable == nullptr) {
        return false;
    }
    auto &exceptionControl = GetExceptionCntrl();

    // Perhaps not needed..
    expControlBlock.flag = exception;
    // Save current registers
    expControlBlock.registersBefore = registers;
    // Move the ISR type to a register...
    registers.dataRegisters[0].data.longword = exception;
    // Note: take this depending on the exception type...
    registers.instrPointer.data.longword = isrVectorTable->exp_illegal_instr;
    // Update the state
    expControlBlock.state = CPUExceptionState::Executing;

    SetActiveException(exception);
    return true;
}
