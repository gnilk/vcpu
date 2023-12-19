//
// Created by gnilk on 16.12.23.
//

#include "fmt/format.h"
#include "CPUBase.h"


using namespace gnilk;
using namespace gnilk::vcpu;

bool CPUBase::RegisterSysCall(uint16_t id, const std::string &name, SysCallDelegate handler) {
    if (syscalls.contains(id)) {
        fmt::println(stderr, "SysCall with id {} ({:#x}) already exists",  id,id);
        return false;
    }
    auto inst = SysCall::Create(id, name, handler);
    syscalls[id] = inst;
    return true;
}
