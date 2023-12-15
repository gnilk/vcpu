//
// Created by gnilk on 14.12.23.
//

#include <stdlib.h>
#include <functional>
#include "fmt/format.h"
#include <limits>
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
    auto szAddressing = FetchByteFromInstrPtr();
    auto dstRegAndFlags = FetchByteFromInstrPtr();
    auto srcRegAndFlags = FetchByteFromInstrPtr();


    auto dstAdrMode = static_cast<AddressMode>(dstRegAndFlags & 0x0f);
    auto srcAdrMode = static_cast<AddressMode>(srcRegAndFlags & 0x0f);

    auto dstReg = (dstRegAndFlags>>4) & 15;
    auto srcReg = (srcRegAndFlags>>4) & 15;


    auto opClass = nextOperand;
    switch(opClass) {
        case BRK :
            // halt here
            return false;
            break;
        case MOV :
            ExecuteMoveInstr(static_cast<OperandSize>(szAddressing),dstAdrMode, dstReg, srcAdrMode, srcReg);
            break;
        case ADD :
            ExecuteAddInstr(static_cast<OperandSize>(szAddressing),dstAdrMode, dstReg, srcAdrMode, srcReg);
        case PUSH :
            break;
    }
    return true;
}

void VirtualCPU::ExecuteMoveInstr(OperandSize szOperand, AddressMode dstAddrMode, int idxDstRegister, AddressMode srcAddrMode, int idxSrcRegister) {

    auto v = ReadFromSrc(szOperand, srcAddrMode, idxSrcRegister);

    if (dstAddrMode == AddressMode::Register) {
        RegisterValue &reg = idxDstRegister>7?registers.addressRegisters[idxDstRegister-8]:registers.dataRegisters[idxDstRegister];
        reg.data = v.data;

        // FIXME: Update CPU flags here...
    }
}

//
// This tries to implement add/sub handling with CPU status updates at the same time
// The idea was to minize code-duplication due to register layout - but I couldn't really figure out
// a good way...
//


#define VCPU_MAX_BYTE     std::numeric_limits<uint8_t>::max()
#define VCPU_MAX_WORD     std::numeric_limits<uint16_t>::max()
#define VCPU_MAX_DWORD    std::numeric_limits<uint32_t>::max()
#define VCPU_MAX_LWORD   std::numeric_limits<uint64_t>::max()

#define chk_add_overflow(__max__,__a__,__b__) (__b__ > (__max__ - __a__))
#define chk_sub_underflow(__a__, __b__) (__b__ > __a__)
#define chk_sub_zero(__a__, __b__) (__b__== __a__)

using OperandDelegate = std::function<CPUStatusFlags(RegisterValue &dst, const RegisterValue &src)>;
static std::unordered_map<OperandSize, OperandDelegate> adders = {
    {OperandSize::Byte, [](RegisterValue &dst, const RegisterValue &src) {
        CPUStatusFlags flags = chk_add_overflow(VCPU_MAX_BYTE, dst.data.byte, src.data.byte) << CPUStatusFlagBitPos::Overflow;
        dst.data.byte += src.data.byte;
        return flags;
    }},
    {OperandSize::Word, [](RegisterValue &dst, const RegisterValue &src) {
        CPUStatusFlags flags = chk_add_overflow(VCPU_MAX_WORD, dst.data.byte, src.data.byte) << CPUStatusFlagBitPos::Overflow;
        dst.data.word += src.data.word;
        return flags;
    }},
    {OperandSize::DWord, [](RegisterValue &dst, const RegisterValue &src) {
        CPUStatusFlags flags = chk_add_overflow(VCPU_MAX_DWORD, dst.data.byte, src.data.byte) << CPUStatusFlagBitPos::Overflow;
        dst.data.dword += src.data.dword;
        return flags;
    }},
    {OperandSize::Long, [](RegisterValue &dst, const RegisterValue &src) {
        CPUStatusFlags flags = chk_add_overflow(VCPU_MAX_LWORD, dst.data.byte, src.data.byte) << CPUStatusFlagBitPos::Overflow;
        dst.data.longword += src.data.longword;
        return flags;
    }}
};
static std::unordered_map<OperandSize, OperandDelegate> subbers = {
    {OperandSize::Byte, [](RegisterValue &dst, const RegisterValue &src) {
        CPUStatusFlags flags;
        flags = chk_sub_underflow(dst.data.byte, src.data.byte) << CPUStatusFlagBitPos::Underflow;
        flags |= chk_sub_zero(dst.data.byte, src.data.byte) << CPUStatusFlagBitPos::Zero;
        dst.data.byte -= src.data.byte;
        return flags;
    }},
    {OperandSize::Word, [](RegisterValue &dst, const RegisterValue &src) {
        CPUStatusFlags flags;
        flags = chk_sub_underflow(dst.data.word, src.data.word) << CPUStatusFlagBitPos::Underflow;
        flags |= chk_sub_zero(dst.data.word, src.data.word) << CPUStatusFlagBitPos::Zero;
        dst.data.word -= src.data.word;
        return flags;
    }},
    {OperandSize::DWord, [](RegisterValue &dst, const RegisterValue &src) {
        CPUStatusFlags flags;
        flags = chk_sub_underflow(dst.data.dword, src.data.dword) << CPUStatusFlagBitPos::Underflow;
        flags |= chk_sub_zero(dst.data.dword, src.data.dword) << CPUStatusFlagBitPos::Zero;
        dst.data.dword -= src.data.dword;
        return flags;
    }},
    {OperandSize::Long, [](RegisterValue &dst, const RegisterValue &src) {
        CPUStatusFlags flags;
        flags = chk_sub_underflow(dst.data.longword, src.data.longword) << CPUStatusFlagBitPos::Underflow;
        flags |= chk_sub_zero(dst.data.longword, src.data.longword) << CPUStatusFlagBitPos::Zero;
        dst.data.longword -= src.data.longword;
        return flags;
    }}
};

static CPUStatusFlags Add(OperandSize opSz, RegisterValue &dst, const RegisterValue &src) {
    if (!adders.contains(opSz)) {
        return CPUStatusFlags::None;
    }
    return adders[opSz](dst, src);
}

static CPUStatusFlags Sub(OperandSize opSz, RegisterValue &dst, const RegisterValue &src) {
    if (!adders.contains(opSz)) {
        return CPUStatusFlags::None;
    }
    return subbers[opSz](dst, src);
}


void VirtualCPU::ExecuteAddInstr(OperandSize szOperand, AddressMode dstAddrMode, int idxDstRegister, AddressMode srcAddrMode, int idxSrcRegister) {
    auto v = ReadFromSrc(szOperand, srcAddrMode, idxSrcRegister);

    if (dstAddrMode == AddressMode::Register) {
        RegisterValue &dstReg = idxDstRegister>7?registers.addressRegisters[idxDstRegister-8]:registers.dataRegisters[idxDstRegister];
        CPUStatusFlags newFlags = Add(szOperand, dstReg, v);
        statusReg.eflags |= newFlags;
    }
}

void VirtualCPU::ExecuteSubInstr(OperandSize szOperand, AddressMode dstAddrMode, int idxDstRegister, AddressMode srcAddrMode, int idxSrcRegister) {
    auto v = ReadFromSrc(szOperand, srcAddrMode, idxSrcRegister);

    if (dstAddrMode == AddressMode::Register) {
        RegisterValue &reg = idxDstRegister>7?registers.addressRegisters[idxDstRegister-8]:registers.dataRegisters[idxDstRegister];
        // Update arithmetic flags based on this operation
        statusReg.eflags |= Sub(szOperand, reg, v);
    }
}
void VirtualCPU::ExecuteMulInstr(OperandSize szOperand, AddressMode dstAddrMode, int idxDstRegister, AddressMode srcAddrMode, int idxSrcRegister) {

}
void VirtualCPU::ExecuteDivInstr(OperandSize szOperand, AddressMode dstAddrMode, int idxDstRegister, AddressMode srcAddrMode, int idxSrcRegister) {

}



RegisterValue VirtualCPU::ReadFromSrc(OperandSize szOperand, AddressMode srcAddrMode, int idxSrcRegister) {
    // Handle immediate mode
    RegisterValue v = {};

    if (srcAddrMode == AddressMode::Immediate) {
        return ReadSrcImmediateMode(szOperand);
    } else if (srcAddrMode == AddressMode::Register) {
        RegisterValue &reg = idxSrcRegister>7?registers.addressRegisters[idxSrcRegister-8]:registers.dataRegisters[idxSrcRegister];
        v.data = reg.data;
    }

    return v;
}

RegisterValue VirtualCPU::ReadSrcImmediateMode(OperandSize szOperand) {
    RegisterValue v = {};
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
    return v;
}
