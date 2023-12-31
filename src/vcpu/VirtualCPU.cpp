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

    auto instrDecoder = InstructionDecoder::Create(GetInstrPtr().data.longword);
    // Tell decoder to decode the basics of this instruction
    if (!instrDecoder->Decode(*this)) {
        return false;
    }
    // Advance forward..
    AdvanceInstrPtr(instrDecoder->GetInstrSizeInBytes());

    //
    // This would be cool:
    // Also, we should put enough information in the first 2-3 bytes to understand the fully decoded size
    // This way we can basically have multiple threads decoding instructions -> super scalar emulation
    //
    switch(instrDecoder->opClass) {
        case BRK :
            fmt::println(stderr, "BRK - CPU Halted!");
            // raise halt exception
            return false;
        case NOP :
            break;
        case SYS :
            ExecuteSysCallInstr(instrDecoder);
        case CALL :
            ExecuteCallInstr(instrDecoder);
            break;
        case LEA :
            ExecuteLeaInstr(instrDecoder);
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
        case LSR :
            ExecuteLsrInstr(instrDecoder);
            break;
        case LSL :
            ExecuteLslInstr(instrDecoder);
            break;
        case ASR :
            ExecuteAsrInstr(instrDecoder);
            break;
        case ASL :
            ExecuteAslInstr(instrDecoder);
            break;
        default:
            // raise invaild-instr. exception here!
            fmt::println(stderr, "Invalid operand: {}", instrDecoder->opCode);
            return false;
    }
    lastDecodedInstruction = instrDecoder;
    return true;
}

// Helpers
template<typename T>
static uint64_t MSBForTypeInReg(RegisterValue &v) {
    // Return the MSB depending on the op.size we are dealing with...
    return (v.data.longword & (uint64_t(1) << (std::numeric_limits<T>::digits - 1)));
}
static uint64_t MSBForOpSize(OperandSize szOperand, RegisterValue &v) {
    switch(szOperand) {
        case OperandSize::Byte :
            return  MSBForTypeInReg<uint8_t>(v);
        case OperandSize::Word :
            return MSBForTypeInReg<uint16_t>(v);
        case OperandSize::DWord :
            return MSBForTypeInReg<uint32_t>(v);
        case OperandSize::Long :
            return MSBForTypeInReg<uint64_t>(v);
    }
    // Should never happen
    return 0;
}
// Update the CPU flags
template<typename T>
static void UpdateCPUFlags(CPUStatusReg &statusReg, uint64_t numRes, uint64_t numDst, uint64_t numSrc) {
    // Flag computation
    auto flag_n = (numRes >> (std::numeric_limits<T>::digits-1)) & 1;
    auto flag_c = numSrc > (std::numeric_limits<T>::max() - numDst);
    auto flag_x = flag_c;
    auto flag_z = !(numRes);

    statusReg.flags.carry = flag_c;
    statusReg.flags.extend = flag_x;
    statusReg.flags.zero = flag_z;
    statusReg.flags.negative = flag_n;
}

void VirtualCPU::ExecuteAslInstr(InstructionDecoder::Ref instrDecoder) {
    auto v = instrDecoder->GetValue();
    if (instrDecoder->dstAddrMode != AddressMode::Register) {
        // raise exception
        return;
    }

    auto &dstReg = GetRegisterValue(instrDecoder->dstRegIndex);

    auto signBefore = MSBForOpSize(instrDecoder->szOperand, dstReg);

    uint64_t msb = 0;
    bool isZero = false;
    int nBits = dstReg.data.byte;
    while(nBits > 0) {
        // Will we shift out something??
        msb = MSBForOpSize(instrDecoder->szOperand, dstReg);
        switch(instrDecoder->szOperand) {
            case OperandSize::Byte :
                dstReg.data.byte = dstReg.data.byte <<  1;
                if (!dstReg.data.byte) isZero = true;
                break;
            case OperandSize::Word :
                dstReg.data.word = dstReg.data.word <<  1;
                if (!dstReg.data.word) isZero = true;
                break;
            case OperandSize::DWord :
                dstReg.data.dword = dstReg.data.dword <<  1;
                if (!dstReg.data.dword) isZero = true;
                break;
            case OperandSize::Long :
                dstReg.data.longword = dstReg.data.longword << 1;
                if (!dstReg.data.longword) isZero = true;
                break;
        }
        nBits--;
    }
    // Only update these in case of non-zero shifts
    if (msb) {
        statusReg.flags.carry = true;
        statusReg.flags.extend = true;
    } else {
        statusReg.flags.carry = false;
        statusReg.flags.carry = false;
    }
    statusReg.flags.zero = isZero;
    statusReg.flags.negative = (msb)?true:false;
    // Overflow - IF the MSB is toggled during anypoint...
    if (signBefore != MSBForOpSize(instrDecoder->szOperand, dstReg)) {
        statusReg.flags.overflow = true;
    } else {
        statusReg.flags.overflow = false;
    }
}

// This can be simplified...
// See: MoiraALU_cpp.h - could take a few impl. ideas from there..
void VirtualCPU::ExecuteAsrInstr(InstructionDecoder::Ref instrDecoder) {
    auto v = instrDecoder->GetValue();
    if (instrDecoder->dstAddrMode != AddressMode::Register) {
        // raise exception
        return;
    }
    auto &dstReg = GetRegisterValue(instrDecoder->dstRegIndex);

    // Fetch the sign-bit..
    uint64_t msb = MSBForOpSize(instrDecoder->szOperand, dstReg);

    bool carryExtFlag = false;
    bool isZero = false;
    // Loop is needed since we propagate the MSB
    // All this is done 'unsigned' during emulating since signed shift is UB..
    int nBits = v.data.byte;
    while(nBits) {
        carryExtFlag = (dstReg.data.longword & 1)?true:false;
        switch(instrDecoder->szOperand) {
            case OperandSize::Byte :
                dstReg.data.byte = dstReg.data.byte >>  1;
                if (!dstReg.data.byte) isZero = true;
                break;
            case OperandSize::Word :
                dstReg.data.word = dstReg.data.word >>  1;
                if (!dstReg.data.word) isZero = true;
                break;
            case OperandSize::DWord :
                dstReg.data.dword = dstReg.data.dword >>  1;
                if (!dstReg.data.dword) isZero = true;
                break;
            case OperandSize::Long :
                dstReg.data.longword = dstReg.data.longword >>  1;
                if (!dstReg.data.longword) isZero = true;
                break;
        }
        // Mask in the 'sign'-bit (MSB) for the correct type
        dstReg.data.longword |= msb;
        nBits--;
    }
    // Only update these in case of non-zero shifts
    if (v.data.byte > 0) {
        statusReg.flags.carry = carryExtFlag;
        statusReg.flags.extend = carryExtFlag;
    } else {
        statusReg.flags.carry = false;
    }
    statusReg.flags.zero = isZero;
    statusReg.flags.negative = (msb)?true:false;
    // Overflow flag - is a bit odd - might not apply to ASR but rather to ASL...
    // see m68000prm.pdf - page. 4-22
    // V — Set if the most significant bit is changed at any time during the shift operation;
    // cleared otherwise.
}


template<OperandSize szOp>
uint64_t GetRegVal(RegisterValue &regValue) {
    switch(szOp) {
        case OperandSize::Byte :
            return regValue.data.byte;
        case OperandSize::Word :
            return regValue.data.word;
        case OperandSize::DWord :
            return regValue.data.dword;
        case OperandSize::Long :
            return regValue.data.longword;
    }
}
template<OperandSize szOp>
void SetRegVal(RegisterValue &reg, uint64_t value) {
    switch(szOp) {
        case OperandSize::Byte :
            reg.data.byte = (uint8_t)value;
            break;
        case OperandSize::Word :
            reg.data.word = (uint16_t)value;
            break;
        case OperandSize::DWord :
            reg.data.dword = (uint32_t)value;
            break;
        case OperandSize::Long :
            reg.data.longword = (uint64_t)value;
            break;
    }
}

template<OperandSize szOp, OperandClass opCode>
void Shift(CPUStatusReg &status, int cnt, RegisterValue &regValue) {
    // FIXME: handle all shift operations here (ASR/LSR/ASL/LSL/ROL/ROR)
    if (opCode == OperandClass::LSL) {
        auto v = GetRegVal<szOp>(regValue);
        for(int i=0;i<cnt;i++) {
            v = v << 1;
        }
        SetRegVal<szOp>(regValue, v);
        // FIXME: Update flags
    }
}


// FIXME: Verify and update CPU Status Flags
void VirtualCPU::ExecuteLslInstr(InstructionDecoder::Ref instrDecoder) {
    auto v = instrDecoder->GetValue();
    if (instrDecoder->dstAddrMode != AddressMode::Register) {
        // raise exception
        return;
    }
    auto &dstReg = GetRegisterValue(instrDecoder->dstRegIndex);

    // I would like to get rid of this switch, but I can't without having an encapsulation class
    switch(instrDecoder->szOperand) {
        case OperandSize::Byte :
            Shift<OperandSize::Byte, OperandClass::LSL>(statusReg, v.data.byte, dstReg);
            break;
        case OperandSize::Word :
            Shift<OperandSize::Word, OperandClass::LSL>(statusReg, v.data.byte, dstReg);
            break;
        case OperandSize::DWord :
            Shift<OperandSize::DWord, OperandClass::LSL>(statusReg, v.data.byte, dstReg);
            break;
        case OperandSize::Long :
            Shift<OperandSize::Long, OperandClass::LSL>(statusReg, v.data.byte, dstReg);
            break;
    }
}

// FIXME: Verify and CPU Status flags
void VirtualCPU::ExecuteLsrInstr(InstructionDecoder::Ref instrDecoder) {
    auto v = instrDecoder->GetValue();
    if (instrDecoder->dstAddrMode != AddressMode::Register) {
        // raise exception
        return;
    }
    auto &dstReg = GetRegisterValue(instrDecoder->dstRegIndex);

    switch(instrDecoder->szOperand) {
        case OperandSize::Byte :
            // FIXME: Make sure this works as expected!
            dstReg.data.byte = dstReg.data.byte >>  v.data.byte;
            break;
        case OperandSize::Word :
            dstReg.data.word = dstReg.data.word >>  v.data.byte;
            break;
        case OperandSize::DWord :
            dstReg.data.dword = dstReg.data.dword >>  v.data.byte;
            break;
        case OperandSize::Long :
            dstReg.data.longword = dstReg.data.longword >> v.data.byte;
            break;
    }
}

//
// Move of these will be small - consider supporting lambda in description code instead...
//
void VirtualCPU::ExecuteSysCallInstr(InstructionDecoder::Ref instrDecoder) {
    auto id = registers.dataRegisters[0].data.word;
    if (syscalls.contains(id)) {
        syscalls.at(id)->Invoke(registers, this);
    }
}

void VirtualCPU::ExecutePushInstr(InstructionDecoder::Ref instrDecoder) {
    auto &v = instrDecoder->GetValue();
    stack.push(v);
}

void VirtualCPU::ExecutePopInstr(InstructionDecoder::Ref instrDecoder) {
    auto v = stack.top();
    stack.pop();
    WriteToDst(instrDecoder, v);
}

void VirtualCPU::ExecuteLeaInstr(InstructionDecoder::Ref instrDecoder) {
    auto &v = instrDecoder->GetValue();
    WriteToDst(instrDecoder, v);
}


void VirtualCPU::ExecuteMoveInstr(InstructionDecoder::Ref instrDecoder) {
    auto &v = instrDecoder->GetValue();
    WriteToDst(instrDecoder, v);
}

//
// Add/Sub helpers with CPU status updates
// The idea was to minize code-duplication due to register layout - not sure if this is a good way..
//
//


//
// Overflow logic taken from Musashi (https://github.com/kstenerud/Musashi/)
//
template<typename T>
static void UpdateOverflowForAdd(CPUStatusReg &statusReg, uint64_t numRes, uint64_t numDst, uint64_t numSrc) {
    // #define VFLAG_ADD_8(S, D, R) ((S^R) & (D^R))
    // where: S = src, D = dst, R = res
    auto flag_v = (((numSrc ^ numRes) & (numDst ^ numRes)) >> (std::numeric_limits<T>::digits-1)) & 1;
    statusReg.flags.overflow = flag_v;
}
template<typename T>
static void UpdateOverflowForSub(CPUStatusReg &statusReg, uint64_t numRes, uint64_t numDst, uint64_t numSrc) {
    // #define VFLAG_SUB_8(S, D, R) ((S^D) & (R^D))
    // where: S = src, D = dst, R = res
    auto flag_v = (((numSrc ^ numDst) & (numRes ^ numDst)) >> (std::numeric_limits<T>::digits-1)) & 1;
    statusReg.flags.overflow = flag_v;
}

// this should work for 8,16,32,64 bits
template<typename T, std::enable_if_t<std::is_integral<T>::value, bool> = true >
static void AddValues(CPUStatusReg &statusReg, RegisterValue &dst, const RegisterValue &src) {
    RegisterValue res;

    auto numSrc = src.data.longword & (std::numeric_limits<T>::max());
    auto numDst = dst.data.longword & (std::numeric_limits<T>::max());
    auto numRes = numSrc + numDst;

    UpdateCPUFlags<T>(statusReg, numRes, numDst, numSrc);
    UpdateOverflowForAdd<T>(statusReg, numRes, numDst, numSrc);

    // preserve whatever was in the register before operation...

    uint64_t mask = (std::numeric_limits<uint64_t>::max()) ^ (std::numeric_limits<T>::max());
    dst.data.longword = dst.data.longword & mask;
    dst.data.longword |= (numRes & (std::numeric_limits<T>::max()));
}

// this should work for 8,16,32,64 bits
template<typename T, std::enable_if_t<std::is_integral<T>::value, bool> = true >
static void SubtractValues(CPUStatusReg &statusReg, RegisterValue &dst, const RegisterValue &src) {
    RegisterValue res;

    auto numSrc = src.data.longword & (std::numeric_limits<T>::max());
    auto numDst = dst.data.longword & (std::numeric_limits<T>::max());
    auto numRes = numDst - numSrc;

    UpdateCPUFlags<T>(statusReg, numRes, numDst, numSrc);
    UpdateOverflowForSub<T>(statusReg, numRes, numDst, numSrc);

    // preserve whatever was in the register before operation...

    uint64_t mask = (std::numeric_limits<uint64_t>::max()) ^ (std::numeric_limits<T>::max());
    dst.data.longword = dst.data.longword & mask;
    dst.data.longword |= (numRes & (std::numeric_limits<T>::max()));
}


void VirtualCPU::ExecuteAddInstr(InstructionDecoder::Ref instrDecoder) {
    auto &v = instrDecoder->GetValue();

    if (instrDecoder->dstAddrMode == AddressMode::Register) {
        auto &dstReg = GetRegisterValue(instrDecoder->dstRegIndex);

        switch(instrDecoder->szOperand) {
            case OperandSize::Byte :
                AddValues<uint8_t>(statusReg, dstReg, v);
                break;
            case OperandSize::Word :
                AddValues<uint16_t>(statusReg, dstReg, v);
                break;
            case OperandSize::DWord :
                AddValues<uint32_t>(statusReg, dstReg, v);
                break;
            case OperandSize::Long :
                AddValues<uint64_t>(statusReg, dstReg, v);
                break;
        }
    }
}

void VirtualCPU::ExecuteSubInstr(InstructionDecoder::Ref instrDecoder) {
    auto &v = instrDecoder->GetValue();

    if (instrDecoder->dstAddrMode == AddressMode::Register) {
        auto &dstReg = GetRegisterValue(instrDecoder->dstRegIndex);
        switch(instrDecoder->szOperand) {
            case OperandSize::Byte :
                SubtractValues<uint8_t>(statusReg, dstReg, v);
                break;
            case OperandSize::Word :
                SubtractValues<uint16_t>(statusReg, dstReg, v);
                break;
            case OperandSize::DWord :
                SubtractValues<uint32_t>(statusReg, dstReg, v);
                break;
            case OperandSize::Long :
                SubtractValues<uint64_t>(statusReg, dstReg, v);
                break;
        }
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
        case OperandSize::Byte :
            registers.instrPointer.data.longword += v.data.byte;
            break;
        case OperandSize::Word :
            registers.instrPointer.data.longword += v.data.word;
            break;
        case OperandSize::DWord :
            registers.instrPointer.data.longword += v.data.dword;
            break;
        case OperandSize::Long :
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

