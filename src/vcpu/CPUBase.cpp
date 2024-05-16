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

    // NOTE: The System Block is not initialized in this...
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

    systemBlock  = (MemoryLayout *)ram;

    isrVectorTable = &systemBlock->isrVectorTable; //(ISR_VECTOR_TABLE*)ram;
    isrControlBlocks = systemBlock->isrControlBlocks;   // array - no need to grab pointer...
    expControlBlock = &systemBlock->exceptionControlBlock;

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

bool CPUBase::AddPeripheral(CPUIntFlag intMask, CPUInterruptId interruptId, Peripheral::Ref peripheral) {

    if (systemBlock == nullptr) {
        fmt::println(stderr, "CPU started with 'QuickStart' - no support for peripherals, use 'Begin' to properly setup interrupts and other advanced features");
        return false;
    }

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

    return true;
}

void CPUBase::ResetPeripherals() {
    for(auto &p : peripherals) {
        p.peripheral->Initialize();
    }
}

void CPUBase::UpdatePeripherals() {
    // This makes Peripherals run in sync with the CPU - but that's not quite true what they do
    // Instead they run based on electrical signals and the CPU samples them

    // Still, keeping this interface in case I did something stupid..
    for(auto &p : peripherals) {
        p.peripheral->Update();
    }
}

void CPUBase::RaiseInterrupt(CPUInterruptId interruptId) {

    // Can't do this unless we have been started with 'Begin'...
    if (systemBlock == nullptr) {
        return;
    }

    if (GetISRState(interruptId) != CPUISRState::Waiting) {
        // Already within an ISR - do NOT execute another
        return;
    }

    // FIXME: Lock ISR handling here!

    // Is this mapped??
    if (interruptMapping.find(interruptId) == interruptMapping.end()) {
        return;
    }

    auto mask = interruptMapping[interruptId];
    auto &intCntrl = GetInterruptCntrl();

    // Is this enabled?
    if (!(intCntrl.data.bits & mask)) {
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

void CPUBase::EnableInterrupt(CPUIntFlag interrupt) {
    // This is a developer problem...
    if (systemBlock == nullptr) {
        fmt::println(stderr, "CPUBase, started with 'QuickStart' no exception handling, use 'Begin' to get advanced features");
        exit(1);
    }

    auto &intCntrl = GetInterruptCntrl();
    intCntrl.data.bits |= interrupt;
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

        if ((intCntrl.data.bits & (1<<i)) && (isrControlBlock.isrState == CPUISRState::Flagged)) {
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
bool CPUBase::RaiseException(CPUExceptionId exceptionId) {
    if (systemBlock == nullptr) {
        fmt::println(stderr, "CPUBase, started with 'QuickStart' no exception handling, use 'Begin' to get advanced features");
        return false;
    }

    if (IsCPUExpActive()) {
        fmt::println(stderr, "CPUBase, nested exception detected, halting CPU");
        Halt();
        return false;
    }

    // Is it enabled?
    if (!IsExceptionEnabled(exceptionId)) {
        fmt::println(stderr, "CPUBase, exception not enabled for {:#x} - halting cpu", (int)exceptionId);
        Halt();
        return false;
    }

    fmt::println(stderr, "CPUBase, raising exception - {:#x}", (int)exceptionId);

    expControlBlock->state = CPUExceptionState::Raised;
    return InvokeExceptionHandlers(exceptionId);
}

// Helper
static constexpr CPUExceptionFlag CPUExpIdToFlag(CPUExceptionId exceptionId) {
    return (CPUExceptionFlag)(static_cast<uint64_t>(1) << exceptionId);
}


bool CPUBase::IsExceptionEnabled(CPUExceptionId  exceptionId) {
    auto &exceptionControl = GetExceptionCntrl();
    return (exceptionControl.data.longword & CPUExpIdToFlag(exceptionId));
}


void CPUBase::EnableException(CPUExceptionId exceptionId) {
    // This is a developer problem...
    if (systemBlock == nullptr) {
        fmt::println(stderr, "CPUBase, started with 'QuickStart' no exception handling, use 'Begin' to get advanced features");
        exit(1);
    }

    auto &exceptionControl = GetExceptionCntrl();
    exceptionControl.data.longword |= CPUExpIdToFlag(exceptionId);
}

void CPUBase::SetActiveException(CPUExceptionId exceptionId) {
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
    expControlBlock->state = CPUExceptionState::Idle;
}

bool CPUBase::InvokeExceptionHandlers(CPUExceptionId exceptionId)  {
    if (systemBlock == nullptr) {
        return false;
    }
    auto &exceptionControl = GetExceptionCntrl();

    // Perhaps not needed..
    expControlBlock->flag = CPUExpIdToFlag(exceptionId);
    // Save current registers
    expControlBlock->registersBefore = registers;
    // Move the ISR type to a register...
    registers.dataRegisters[0].data.longword = exceptionId;
    // Note: take this depending on the exception type...
    registers.instrPointer.data.longword = isrVectorTable->exp_illegal_instr;
    // Update the state
    expControlBlock->state = CPUExceptionState::Executing;

    SetActiveException(exceptionId);
    return true;
}
