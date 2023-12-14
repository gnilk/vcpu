//
// Created by gnilk on 14.12.23.
//

#include <stdlib.h>
#include "fmt/format.h"
#include "VirtualCPU.h"

using namespace gnilk;

bool VirtualCPU::Step() {
    auto nextOperand = FetchByteFromInstrPtr();
    if (nextOperand == 0xff) {
        return false;
    }

    if (nextOperand >= 0xe0) {
        // handle differently
        return false;
    }
    //
    // Setup addressing
    //
    auto addressing = FetchByteFromInstrPtr();
    auto dstRegister = (addressing & 0xf0) >> 4;
    auto srcdstflags = (addressing & 0x0f);

    auto dstAdrMode = static_cast<AddressMode>(srcdstflags >> 2);
    auto srcAdrMode = static_cast<AddressMode>(srcdstflags & 0x03);



    auto opClass = nextOperand & 0xf0;
    auto szAddressing = static_cast<OperandSize>(nextOperand & 0x03);
    switch(opClass) {
        case BRK :
            // halt here
            return false;
            break;
        case MOV :
            ExecuteMoveInstr(szAddressing,dstAdrMode, dstRegister, srcAdrMode);
            break;
        case PUSH :
            break;
    }
    return true;
}

void VirtualCPU::ExecuteMoveInstr(OperandSize szOperand, AddressMode dstAddrMode, int idxDstRegister, AddressMode srcAddrMode) {

    RegisterValue v;
    if (srcAddrMode == AddressMode::Immediate) {
        switch(szOperand) {
            case OperandSize::Byte :
                v.data.byte = FetchFromInstrPtr<uint8_t>();
                break;
            case OperandSize::Word :
                v.data.word = FetchFromInstrPtr<uint16_t>();
                break;
            case OperandSize::DWord :
                v.data.dword = FetchFromInstrPtr<uint32_t>();
                break;
            case OperandSize::Long :
                v.data.longword = FetchFromInstrPtr<uint64_t>();
                break;
        }
    }

    if (dstAddrMode == AddressMode::Register) {
        RegisterValue &reg = idxDstRegister>7?registers.addressRegisters[idxDstRegister-8]:registers.dataRegisters[idxDstRegister];
        reg.data = v.data;

        // FIXME: Update CPU flags here...
    }
}


