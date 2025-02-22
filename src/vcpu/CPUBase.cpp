//
// Created by gnilk on 16.12.23.
//

#include "fmt/format.h"
#include "CPUBase.h"
#include "System.h"
#include <mutex>

using namespace gnilk;
using namespace gnilk::vcpu;

CPUBase::~CPUBase() {
    DoEnd();
}

// QuickStart won't initialize the ISR Table and reserve stuff - it will just assign the RAM ptr
void CPUBase::QuickStart(void* ptrRam, size_t sizeOfRam) {
    memset(&registers, 0, sizeof(registers));

    auto defaultRam = SoC::Instance().GetFirstRegionFromBusType<RamBus>();
    // auto-expand here, in case the default RAM is smaller than the quickstart RAM
    if (defaultRam->szPhysical < sizeOfRam) {
        defaultRam->Resize(sizeOfRam);
    }

    auto copyResult = memoryUnit.CopyToRamFromExt(0, ptrRam, sizeOfRam);
    if (copyResult < 0) {
        fmt::println(stderr, "Err: MMU failed to copy from external memory to internal RAM");
        exit(1);
    }

    // NOTE: The System Block is not initialized in this...
}

void CPUBase::Begin(void* ptrRam, size_t sizeOfRam) {
//    ram = static_cast<uint8_t *>(ptrRam);
//    szRam = sizeOfRam;

    memoryUnit.CopyToRamFromExt(0, ptrRam, sizeOfRam);


    Reset();
}
void CPUBase::Begin() {

    Reset();
}

void CPUBase::End() {
    DoEnd();
}
void CPUBase::DoEnd() {
    DelPeripherals();
}

void CPUBase::Reset() {
    // Everything is zero upon reset...
    memset(&registers, 0, sizeof(registers));

    // Ah - this is interesting - we need to fix this!
    auto ramregion = SoC::Instance().GetFirstRegionFromBusType<RamBus>();
    if (ramregion == nullptr) {
        fmt::println(stderr, "CPUBase::Reset, No RAM configuration!");
        exit(1);
    }
    if (ramregion->ptrPhysical == nullptr) {
        fmt::println(stderr, "CPUBase::Reset, RAM region found - but physical memory not attached (nullptr)");
        exit(1);
    }

    // FIXME: This is not correct
    systemBlock  = (MemoryLayout *)ramregion->ptrPhysical;

    isrVectorTable = &systemBlock->isrVectorTable; //(ISR_VECTOR_TABLE*)ram;
    isrControlBlocks = systemBlock->isrControlBlocks;   // array - no need to grab pointer...
    expControlBlock = &systemBlock->exceptionControlBlock;

    //isrVectorTable->initial_sp = szRam-1;
    isrVectorTable->initial_sp = ramregion->szPhysical-1;
    isrVectorTable->initial_pc = VCPU_INITIAL_PC;

}

//
// Process the dispatch queue
// FIXME: This should be a loop?
//
CPUBase::kProcessDispatchResult CPUBase::ProcessDispatch() {
    DispatchBase::DispatchItemHeader header;
    auto peekResult = dispatcher.Peek(&header);
    if (peekResult < 0) {
        fmt::println(stderr, "[CPU] ProcessDispatch failed while peeking");
        return kProcessDispatchResult::kPeekErr;
    }
    // empty?
    if (peekResult == 0) {
        return kProcessDispatchResult::kEmpty;
    }

    if (!InstructionSetManager::Instance().HaveExtension(header.instrTypeId)) {
        fmt::println(stderr, "[CPU] Instruction set missing for type={:#X}", header.instrTypeId);
        return kProcessDispatchResult::kNoInstrSet;
    }

    auto &instructionSet = InstructionSetManager::Instance().GetExtension(header.instrTypeId);
    auto &impl = instructionSet.GetImplementation();
    auto execResult =  impl.ExecuteInstruction(*this);
    if (!execResult) {
        return kProcessDispatchResult::kExecFailed;
    }
    lastExecuted = impl.DisasmLastInstruction();
    return kProcessDispatchResult::kExecOk;
}

// Used in unit tests - should probably not be here!
void *CPUBase::GetRawPtrToRAM(uint64_t addr) {
    auto &region = SoC::Instance().GetMemoryRegionFromAddress(addr);

    if ((region.vAddrStart > addr) || (region.vAddrEnd < addr)) {
        return nullptr;
    }

    if (region.ptrPhysical == nullptr) {
        return nullptr;
    }

    size_t offset = addr - region.vAddrStart;
    return &static_cast<uint8_t *>(region.ptrPhysical)[offset];
}

MemoryLayout *CPUBase::GetSystemMemoryBlock() {
    return systemBlock;
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

    // Now allow the Peripheral to start, generally a peripheral would kick off internal threads and what not..
    peripheral->Start();

    return true;
}
void CPUBase::DelPeripherals() {
    for(auto &p : peripherals) {
        p.peripheral->Stop();
    }
    // FIXME: Clear interrupt mappings..
    peripherals.clear();
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


    //
    // Let's lock the ISR data
    //
    std::lock_guard<std::mutex> guard(isrLock);

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

    std::lock_guard<std::mutex> guard(isrLock);
    auto &intCntrl = GetInterruptCntrl();
    intCntrl.data.bits |= interrupt;
}

ISRControlBlock *CPUBase::GetActiveISRControlBlock() {
    if (isrControlBlocks == nullptr) {
        return nullptr;
    }
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

    std::lock_guard<std::mutex> guard(isrLock);

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
