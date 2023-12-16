//
// Created by gnilk on 14.12.23.
//

#include <stdlib.h>
#include <functional>
#include "fmt/format.h"
#include <limits>
#include "VirtualCPU.h"

#include "InstructionDecoder.h"
#include "InstructionSet.h"

using namespace gnilk;
using namespace gnilk::vcpu;


bool VirtualCPU::Step() {
    auto nextOperand = FetchByteFromInstrPtr();
    if (nextOperand == 0xff) {
        return false;
    }
    auto opClass = static_cast<OperandClass>(nextOperand);

    auto instrDecoder = InstructionDecoder::Create(opClass);
    if (instrDecoder == nullptr) {
        return false;
    }

    // Tell decoder to decode the basics of this instruction
    instrDecoder->Decode(*this);


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
            ExecuteMoveInstr(instrDecoder);
            break;
        case ADD :
            //ExecuteAddInstr(static_cast<OperandSize>(szAddressing),dstAdrMode, dstReg, srcAdrMode, srcReg);
            ExecuteAddInstr(instrDecoder);
        case PUSH :
            //ExecutePushInstr(static_cast<OperandSize>(szAddressing), dstAdrMode, dstReg);
            ExecutePushInstr(instrDecoder);
            break;
        case POP :
            // ExecutePopInstr(static_cast<OperandSize>(szAddressing), dstAdrMode, dstReg);
            ExecutePopInstr(instrDecoder);
        break;
    }
    return true;
}

//
// Move of these will be small - consider supporting lambda in description code instead...
//
void VirtualCPU::ExecutePushInstr(InstructionDecoder::Ref instrDecoder) {
    auto v = ReadFromSrc(instrDecoder->szOperand, instrDecoder->dstAddrMode, instrDecoder->dstReg);
    stack.push(v);
}

void VirtualCPU::ExecutePopInstr(InstructionDecoder::Ref instrDecoder) {
    auto v = stack.top();
    stack.pop();

    // Note: we should have 'WriteToDst' in the same sense we have a 'ReadFromSrc'
    if (instrDecoder->dstAddrMode == AddressMode::Register) {
        RegisterValue &reg = instrDecoder->dstReg>7?registers.addressRegisters[instrDecoder->dstReg-8]:registers.dataRegisters[instrDecoder->dstReg];
        reg.data = v.data;

        // FIXME: Update CPU flags here
    }

}

void VirtualCPU::ExecuteMoveInstr(InstructionDecoder::Ref instrDecoder) {
    auto v = ReadFromSrc(instrDecoder->szOperand, instrDecoder->srcAddrMode, instrDecoder->srcReg);

    if (instrDecoder->dstAddrMode == AddressMode::Register) {
        RegisterValue &reg = instrDecoder->dstReg>7?registers.addressRegisters[instrDecoder->dstReg-8]:registers.dataRegisters[instrDecoder->dstReg];
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


void VirtualCPU::ExecuteAddInstr(InstructionDecoder::Ref instrDecoder) {
    auto v = ReadFromSrc(instrDecoder->szOperand, instrDecoder->srcAddrMode, instrDecoder->srcReg);

    if (instrDecoder->dstAddrMode == AddressMode::Register) {
        RegisterValue &dstReg = instrDecoder->dstReg>7?registers.addressRegisters[instrDecoder->dstReg-8]:registers.dataRegisters[instrDecoder->dstReg];
        CPUStatusFlags newFlags = Add(instrDecoder->szOperand, dstReg, v);
        // Did we generate a carry?  Extend bit should set to same
        if ((newFlags & CPUStatusFlags::Carry) == CPUStatusFlags::Carry) {
            newFlags |= CPUStatusFlags::Extend;
        }

        // replace old arithmetic flags
        statusReg.eflags = (statusReg.eflags & CPUStatusAritInvMask) | newFlags;
    }
}

void VirtualCPU::ExecuteSubInstr(InstructionDecoder::Ref instrDecoder) {
    auto v = ReadFromSrc(instrDecoder->szOperand, instrDecoder->srcAddrMode, instrDecoder->srcReg);

    if (instrDecoder->dstAddrMode == AddressMode::Register) {
        RegisterValue &reg = instrDecoder->dstReg>7?registers.addressRegisters[instrDecoder->dstReg-8]:registers.dataRegisters[instrDecoder->dstReg];
        // Update arithmetic flags based on this operation
        CPUStatusFlags newFlags = Sub(instrDecoder->szOperand, reg, v);
        // Did we generate a carry?  Extend bit should set to same
        if ((newFlags & CPUStatusFlags::Carry) == CPUStatusFlags::Carry) {
            newFlags |= CPUStatusFlags::Extend;
        }

        // replace old arithmetic flags
        statusReg.eflags = (statusReg.eflags & CPUStatusAritInvMask) | newFlags;
    }
}
void VirtualCPU::ExecuteMulInstr(InstructionDecoder::Ref instrDecoder) {

}
void VirtualCPU::ExecuteDivInstr(InstructionDecoder::Ref instrDecoder) {

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
