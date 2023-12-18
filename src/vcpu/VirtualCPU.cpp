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

    //
    // This would be cool:
    // Also, we should put enough information in the first 2-3 bytes to understand the fully decoded size
    // This way we can basically have multiple threads decoding instructions -> super scalar emulation
    //
    switch(opClass) {
        case BRK :
            fmt::println(stderr, "BRK - CPU Halted!");
            // raise halt exception
            return false;
        case NOP :
            break;
        case CALL :
            ExecuteCallInstr(instrDecoder);
            break;
        case RET :
            ExecuteRetInstr(instrDecoder);
            break;
        case MOV :
            ExecuteMoveInstr(instrDecoder);
            break;
        case ADD :
            ExecuteAddInstr(instrDecoder);
            break;
        case PUSH :
            ExecutePushInstr(instrDecoder);
            break;
        case POP :
            ExecutePopInstr(instrDecoder);
            break;
        default:
            // raise invaild-instr. exception here!
            fmt::println(stderr, "Invalid operand: {}", nextOperand);
            return false;
    }
    lastDecodedInstruction = instrDecoder;
    return true;
}

//
// Move of these will be small - consider supporting lambda in description code instead...
//
void VirtualCPU::ExecutePushInstr(InstructionDecoder::Ref instrDecoder) {
    auto &v = instrDecoder->GetValue();
    stack.push(v);
}

void VirtualCPU::ExecutePopInstr(InstructionDecoder::Ref instrDecoder) {
    auto v = stack.top();
    stack.pop();
    WriteToDst(instrDecoder, v);
}

void VirtualCPU::ExecuteMoveInstr(InstructionDecoder::Ref instrDecoder) {
    auto &v = instrDecoder->GetValue();
    WriteToDst(instrDecoder, v);
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

    auto &v = instrDecoder->GetValue();

    if (instrDecoder->dstAddrMode == AddressMode::Register) {
        auto &dstReg = GetRegisterValue(instrDecoder->dstRegIndex);
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
    auto &v = instrDecoder->GetValue();

    if (instrDecoder->dstAddrMode == AddressMode::Register) {
        auto &reg = GetRegisterValue(instrDecoder->dstRegIndex);

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

void VirtualCPU::ExecuteCallInstr(InstructionDecoder::Ref instrDecoder) {
    auto &v = instrDecoder->GetValue();
    auto retAddr = registers.instrPointer;
    retAddr.data.longword += 1;
    // push on stack...
    stack.push(retAddr);

    switch(instrDecoder->szOperand) {
        case Byte :
            registers.instrPointer.data.longword += v.data.byte;
            break;
        case Word :
            registers.instrPointer.data.longword += v.data.word;
            break;
        case DWord :
            registers.instrPointer.data.longword += v.data.dword;
            break;
        case Long :
            registers.instrPointer.data.longword = v.data.longword;     // jumping long-word is absolute!
            break;
    }
}

void VirtualCPU::ExecuteRetInstr(InstructionDecoder::Ref instrDecoder) {
    if (stack.empty()) {
        // CPU exception!
        fmt::println(stderr, "RET - no return address - stack empty!!");
        return;
    }
    auto newInstrAddr = stack.top();
    stack.pop();
    registers.instrPointer.data = newInstrAddr.data;
}

//
// Could be moved to base class
//
void VirtualCPU::WriteToDst(InstructionDecoder::Ref instrDecoder, const RegisterValue &v) {
    // Support more address mode
    if (instrDecoder->dstAddrMode == AddressMode::Register) {
        auto &reg = GetRegisterValue(instrDecoder->dstRegIndex);
        reg.data = v.data;
        // FIXME: Update CPU flags here
    }
}

