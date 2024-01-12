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
    // FIXME: implement this
    auto cr0 = registers.cntrlRegisters[0].data.longword;
    auto cr1 = registers.cntrlRegisters[1].data.longword;
//    memoryUnit.SetControl(cr0);
//    memoryUnit.SetPageTranslationVAddr(cr1);
}
