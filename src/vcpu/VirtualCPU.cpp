//
// Created by gnilk on 14.12.23.
//

#include <stdlib.h>
#include <functional>
#include "fmt/format.h"
#include <limits>
#include "VirtualCPU.h"

using namespace gnilk;
//
// This feature description and table should be made visible across all CPU tools..
// Put it in a a separate place so it can be reused...
//
enum class OperandDescriptionFlags : uint8_t {
    OperandSize = 1,
    TwoOperands = 2,
};
inline constexpr OperandDescriptionFlags operator | (OperandDescriptionFlags lhs, OperandDescriptionFlags rhs) {
    using T = std::underlying_type_t<OperandDescriptionFlags>;
    return static_cast<OperandDescriptionFlags>(static_cast<T>(lhs) | static_cast<T>(rhs));
}
inline constexpr bool operator & (OperandDescriptionFlags lhs, OperandDescriptionFlags rhs) {
    using T = std::underlying_type_t<OperandDescriptionFlags>;
    return (static_cast<T>(lhs) & static_cast<T>(rhs));
}
struct OperandDescription {
    OperandDescriptionFlags features;
};

static std::unordered_map<OperandClass, OperandDescription> operDetails = {
  {OperandClass::MOV,{.features = OperandDescriptionFlags::OperandSize | OperandDescriptionFlags::TwoOperands}},
  {OperandClass::ADD,{.features = OperandDescriptionFlags::OperandSize | OperandDescriptionFlags::TwoOperands}},
  {OperandClass::SUB,{.features = OperandDescriptionFlags::OperandSize | OperandDescriptionFlags::TwoOperands}},
  {OperandClass::PUSH,{.features = OperandDescriptionFlags::OperandSize}},
  {OperandClass::POP,{.features = OperandDescriptionFlags::OperandSize}},
};

bool VirtualCPU::Step() {
    auto nextOperand = FetchByteFromInstrPtr();
    if (nextOperand == 0xff) {
        return false;
    }
    auto opClass = static_cast<OperandClass>(nextOperand);

    if (!operDetails.contains(opClass)) {
        // invalid/unsupported operand
        return false;
    }
    auto opDescription = operDetails[opClass];
    //
    // Setup addressing
    //
    uint8_t szAddressing = 0;   // FIXME: default!!!

    if (opDescription.features & OperandDescriptionFlags::OperandSize) {
        szAddressing = FetchByteFromInstrPtr();
    }

    //
    // Setup src/dst handling
    //
    uint8_t dstRegAndFlags = 0;
    uint8_t srcRegAndFlags = 0;

    if (opDescription.features & OperandDescriptionFlags::TwoOperands) {
        dstRegAndFlags = FetchByteFromInstrPtr();
        srcRegAndFlags = FetchByteFromInstrPtr();
    } else {
        dstRegAndFlags = FetchByteFromInstrPtr();
    }

    // This bit-fiddeling we can do anyway...

    auto dstAdrMode = static_cast<AddressMode>(dstRegAndFlags & 0x0f);
    auto srcAdrMode = static_cast<AddressMode>(srcRegAndFlags & 0x0f);

    auto dstReg = (dstRegAndFlags>>4) & 15;
    auto srcReg = (srcRegAndFlags>>4) & 15;

    // Note: Consider creating an 'InstructionDecoding' object to hold all meta data and pass around
    //       We want to pass on the Description as well - and arguments are already quite hefty here...

    //
    // Also, we should put enough information in the first 2-3 bytes to understand the fully decoded size
    // This way we can basically have multiple threads decoding instructions -> super scalar emulation (would be cool).
    //

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
            ExecutePushInstr(static_cast<OperandSize>(szAddressing), dstAdrMode, dstReg);
            break;
        case POP :
            ExecutePopInstr(static_cast<OperandSize>(szAddressing), dstAdrMode, dstReg);
        break;
    }
    return true;
}

//
// Move of these will be small - consider supporting lambda in description code instead...
//
void VirtualCPU::ExecutePushInstr(OperandSize szOperand, AddressMode pushAddrMode, int idxPushRegister) {
    auto v = ReadFromSrc(szOperand, pushAddrMode, idxPushRegister);
    stack.push(v);
}

void VirtualCPU::ExecutePopInstr(OperandSize szOperand, AddressMode dstAddrMode, int idxDstRegister) {
    auto v = stack.top();
    stack.pop();

    // Note: we should have 'WriteToDst' in the same sense we have a 'ReadFromSrc'
    if (dstAddrMode == AddressMode::Register) {
        RegisterValue &reg = idxDstRegister>7?registers.addressRegisters[idxDstRegister-8]:registers.dataRegisters[idxDstRegister];
        reg.data = v.data;

        // FIXME: Update CPU flags here
    }

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
#define chk_sub_negative(__a__, __b__) (__b__ > __a__)
#define chk_sub_zero(__a__, __b__) (__b__== __a__)
/*
 * If I want to mimic M68k - check out the M68000PRM.pdf page 89 for details...
 * I think Carry, Extend, Zero and Negative are correct - overflow is not at all done..
 */
using OperandDelegate = std::function<CPUStatusFlags(RegisterValue &dst, const RegisterValue &src)>;
static std::unordered_map<OperandSize, OperandDelegate> adders = {
    {OperandSize::Byte, [](RegisterValue &dst, const RegisterValue &src) {
        // Will this addition generate a carry?
        CPUStatusFlags flags = chk_add_overflow(VCPU_MAX_BYTE, dst.data.byte, src.data.byte) << CPUStatusFlagBitPos::Carry;
        // Perform add
        dst.data.byte += src.data.byte;

        // Note: The overflow I don't really understand...

        flags |= (dst.data.byte == 0)?CPUStatusFlags::Zero:CPUStatusFlags::None;
        flags |= (dst.data.byte & 0x80)?CPUStatusFlags::Negative:CPUStatusFlags::None;

        return flags;
    }},
    {OperandSize::Word, [](RegisterValue &dst, const RegisterValue &src) {
        CPUStatusFlags flags = chk_add_overflow(VCPU_MAX_WORD, dst.data.byte, src.data.byte) << CPUStatusFlagBitPos::Carry;
        dst.data.word += src.data.word;

        flags |= (dst.data.word == 0)?CPUStatusFlags::Zero:CPUStatusFlags::None;
        flags |= (dst.data.word & 0x8000)?CPUStatusFlags::Negative:CPUStatusFlags::None;

        return flags;
    }},
    {OperandSize::DWord, [](RegisterValue &dst, const RegisterValue &src) {
        CPUStatusFlags flags = chk_add_overflow(VCPU_MAX_DWORD, dst.data.byte, src.data.byte) << CPUStatusFlagBitPos::Carry;
        dst.data.dword += src.data.dword;
        flags |= (dst.data.dword == 0)?CPUStatusFlags::Zero:CPUStatusFlags::None;
        flags |= (dst.data.dword & 0x80000000)?CPUStatusFlags::Negative:CPUStatusFlags::None;

        return flags;
    }},
    {OperandSize::Long, [](RegisterValue &dst, const RegisterValue &src) {
        CPUStatusFlags flags = chk_add_overflow(VCPU_MAX_LWORD, dst.data.byte, src.data.byte) << CPUStatusFlagBitPos::Carry;
        dst.data.longword += src.data.longword;
        flags |= (dst.data.dword == 0)?CPUStatusFlags::Zero:CPUStatusFlags::None;
        flags |= (dst.data.dword & 0x8000000000000000)?CPUStatusFlags::Negative:CPUStatusFlags::None;
        return flags;
    }}
};
static std::unordered_map<OperandSize, OperandDelegate> subbers = {
    {OperandSize::Byte, [](RegisterValue &dst, const RegisterValue &src) {
        CPUStatusFlags flags = chk_sub_negative(dst.data.byte, src.data.byte) << CPUStatusFlagBitPos::Carry;
        dst.data.byte -= src.data.byte;
        flags |= (dst.data.byte == 0)?CPUStatusFlags::Zero:CPUStatusFlags::None;
        flags |= (dst.data.byte & 0x80)?CPUStatusFlags::Negative:CPUStatusFlags::None;
        return flags;
    }},
    {OperandSize::Word, [](RegisterValue &dst, const RegisterValue &src) {
        CPUStatusFlags flags = chk_sub_negative(dst.data.word, src.data.word) << CPUStatusFlagBitPos::Carry;
        dst.data.word -= src.data.word;
        flags |= (dst.data.word == 0)?CPUStatusFlags::Zero:CPUStatusFlags::None;
        flags |= (dst.data.word & 0x8000)?CPUStatusFlags::Negative:CPUStatusFlags::None;
        return flags;
    }},
    {OperandSize::DWord, [](RegisterValue &dst, const RegisterValue &src) {
        CPUStatusFlags flags = chk_sub_negative(dst.data.dword, src.data.dword) << CPUStatusFlagBitPos::Carry;
        dst.data.dword -= src.data.dword;
        flags |= (dst.data.dword == 0)?CPUStatusFlags::Zero:CPUStatusFlags::None;
        flags |= (dst.data.dword & 0x80000000)?CPUStatusFlags::Negative:CPUStatusFlags::None;

        return flags;
    }},
    {OperandSize::Long, [](RegisterValue &dst, const RegisterValue &src) {
        CPUStatusFlags flags = chk_sub_negative(dst.data.longword, src.data.longword) << CPUStatusFlagBitPos::Carry;
        // flags = chk_sub_negative(dst.data.longword, src.data.longword) << CPUStatusFlagBitPos::Negative;
        // flags |= chk_sub_zero(dst.data.longword, src.data.longword) << CPUStatusFlagBitPos::Zero;
        dst.data.longword -= src.data.longword;
        flags |= (dst.data.longword == 0)?CPUStatusFlags::Zero:CPUStatusFlags::None;
        flags |= (dst.data.longword & 0x8000000000000000)?CPUStatusFlags::Negative:CPUStatusFlags::None;
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
        // Did we generate a carry?  Extend bit should set to same
        if ((newFlags & CPUStatusFlags::Carry) == CPUStatusFlags::Carry) {
            newFlags |= CPUStatusFlags::Extend;
        }

        // replace old arithmetic flags
        statusReg.eflags = (statusReg.eflags & CPUStatusAritInvMask) | newFlags;
    }
}

void VirtualCPU::ExecuteSubInstr(OperandSize szOperand, AddressMode dstAddrMode, int idxDstRegister, AddressMode srcAddrMode, int idxSrcRegister) {
    auto v = ReadFromSrc(szOperand, srcAddrMode, idxSrcRegister);

    if (dstAddrMode == AddressMode::Register) {
        RegisterValue &reg = idxDstRegister>7?registers.addressRegisters[idxDstRegister-8]:registers.dataRegisters[idxDstRegister];
        // Update arithmetic flags based on this operation
        CPUStatusFlags newFlags = Sub(szOperand, reg, v);
        // Did we generate a carry?  Extend bit should set to same
        if ((newFlags & CPUStatusFlags::Carry) == CPUStatusFlags::Carry) {
            newFlags |= CPUStatusFlags::Extend;
        }

        // replace old arithmetic flags
        statusReg.eflags = (statusReg.eflags & CPUStatusAritInvMask) | newFlags;
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
