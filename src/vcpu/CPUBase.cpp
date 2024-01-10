//
// Created by gnilk on 16.12.23.
//

#include "fmt/format.h"
#include "CPUBase.h"


using namespace gnilk;
using namespace gnilk::vcpu;
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
