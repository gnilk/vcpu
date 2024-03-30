//
// Created by gnilk on 27.03.24.
//
// FIX-THIS: Remove inheritence to CPUBase and supply CPUBase as an argument
//

#include "CPUInstructionBase.h"

using namespace gnilk;
using namespace gnilk::vcpu;

//
bool CPUInstructionBase::ExecuteInstruction(InstructionDecoder &decoder) {
    switch(decoder.code.opCode) {
        case BRK :
            fmt::println(stderr, "BRK - CPU Halted!");
            registers.statusReg.flags.halt = 1;
            // Enable this
            // pipeline.Flush();
            break;
        case NOP :
            break;
        case SYS :
            ExecuteSysCallInstr(decoder);
            break;
        case CALL :
            ExecuteCallInstr(decoder);
            break;
        case LEA :
            ExecuteLeaInstr(decoder);
            break;
        case RET :
            ExecuteRetInstr(decoder);
            break;
        case RTI :
            ExecuteRtiInstr(decoder);
            break;
        case MOV :
            ExecuteMoveInstr(decoder);
            break;
        case ADD :
            ExecuteAddInstr(decoder);
            break;
        case PUSH :
            ExecutePushInstr(decoder);
            break;
        case POP :
            ExecutePopInstr(decoder);
            break;
        case LSR :
            ExecuteLsrInstr(decoder);
            break;
        case LSL :
            ExecuteLslInstr(decoder);
            break;
        case ASR :
            ExecuteAsrInstr(decoder);
            break;
        case ASL :
            ExecuteAslInstr(decoder);
            break;
        case CMP :
            ExecuteCmpInstr(decoder);
            break;
        case BEQ :
            ExecuteBeqInstr(decoder);
            break;
        case BNE :
            ExecuteBneInstr(decoder);
            break;
        default:
            // raise invaild-instr. exception here!
            fmt::println(stderr, "Invalid operand: {}", decoder.code.opCodeByte);
            return false;
    }
    return true;
}
////////////////////////////
//
// Instruction emulation begins here
//


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

template<typename T>
static void UpdateCPUFlagsCMP(CPUStatusReg &statusReg, uint64_t numRes, uint64_t numDst, uint64_t numSrc) {
    UpdateCPUFlags<T>(statusReg, numRes, numDst, numSrc);
    // Overflow
    auto ovflow =((numDst ^ numSrc) & (numDst ^ numRes));
    statusReg.flags.overflow = (ovflow >> (std::numeric_limits<T>::digits-1)) & 1;
}

void CPUInstructionBase::ExecuteBneInstr(InstructionDecoder& instrDecoder) {
    if (instrDecoder.opArgDst.addrMode != AddressMode::Immediate) {
        // raise exception
        return;
    }
    // zero must not be set in order to jump
    if (registers.statusReg.flags.zero == 1) {
        return;
    }

    // FIXME: This is same as for BEQ - we could keep these..

    auto v = instrDecoder.GetValue();
    int64_t relativeOffset = 0;

    switch(instrDecoder.code.opSize) {
        case OperandSize::Byte :
            relativeOffset = (int8_t)(v.data.byte);
            break;
        case OperandSize::Word :
            relativeOffset = (int16_t)(v.data.word);
            break;
        case OperandSize::DWord :
            relativeOffset = (int32_t)(v.data.dword);
            break;
        case OperandSize::Long :    // in case of long - this is an absolute address...
            registers.instrPointer.data.longword = v.data.longword;
            return;
    }
    registers.instrPointer.data.longword += relativeOffset;

}
void CPUInstructionBase::ExecuteBeqInstr(InstructionDecoder& instrDecoder) {
    auto v = instrDecoder.GetValue();
    if (instrDecoder.opArgDst.addrMode != AddressMode::Immediate) {
        // raise exception
        return;
    }
    // zero must be in order to jump
    if (registers.statusReg.flags.zero == 0) {
        return;
    }

    int64_t relativeOffset = 0;

    switch(instrDecoder.code.opSize) {
        case OperandSize::Byte :
            relativeOffset = (int8_t)(v.data.byte);
            break;
        case OperandSize::Word :
            relativeOffset = (int16_t)(v.data.word);
            break;
        case OperandSize::DWord :
            relativeOffset = (int32_t)(v.data.dword);
            break;
        case OperandSize::Long :    // in case of long - this is an absolute address...
            registers.instrPointer.data.longword = v.data.longword;
            return;
    }
    registers.instrPointer.data.longword += relativeOffset;
}

void CPUInstructionBase::ExecuteCmpInstr(InstructionDecoder& instrDecoder) {
    auto v = instrDecoder.GetValue();
    if (instrDecoder.opArgDst.addrMode == AddressMode::Immediate) {
        // raise exception
        return;
    }
    RegisterValue dstReg = instrDecoder.ReadDstValue(*this);
/*
    if (instrDecoder.dstAddrMode == AddressMode::Register) {
        dstReg = GetRegisterValue(instrDecoder.dstRegIndex, instrDecoder.opFamily);
    } else if (instrDecoder.dstAddrMode == AddressMode::Absolute) {
        dstReg = ReadFromMemoryUnit(instrDecoder.opSize, instrDecoder.dstAbsoluteAddr);
    } else {
        // raise exception??
        fmt::println("ExecuteCmpInstr, invalid address mode; {}", (int)instrDecoder.dstAddrMode);
        return;
    }
*/

    switch(instrDecoder.code.opSize) {
        case OperandSize::Byte :
            UpdateCPUFlagsCMP<uint8_t>(registers.statusReg, dstReg.data.byte - v.data.byte, dstReg.data.byte, v.data.byte);
            break;
        case OperandSize::Word:
            UpdateCPUFlagsCMP<uint16_t>(registers.statusReg, dstReg.data.word - v.data.word, dstReg.data.word, v.data.word);
            break;
        case OperandSize::DWord :
            UpdateCPUFlagsCMP<uint32_t>(registers.statusReg, dstReg.data.dword - v.data.dword, dstReg.data.dword, v.data.dword);
            break;
        case OperandSize::Long :
            UpdateCPUFlagsCMP<uint64_t>(registers.statusReg, dstReg.data.longword - v.data.longword, dstReg.data.longword, v.data.longword);
            break;
    }
}

void CPUInstructionBase::ExecuteAslInstr(InstructionDecoder& instrDecoder) {
    auto v = instrDecoder.GetValue();
    if (instrDecoder.opArgDst.addrMode != AddressMode::Register) {
        // raise exception
        return;
    }

    auto &dstReg = GetRegisterValue(instrDecoder.opArgDst.regIndex, instrDecoder.code.opFamily);

    auto signBefore = MSBForOpSize(instrDecoder.code.opSize, dstReg);

    uint64_t msb = 0;
    bool isZero = false;
    int nBits = v.data.byte;
    while(nBits > 0) {
        // Will we shift out something??
        msb = MSBForOpSize(instrDecoder.code.opSize, dstReg);
        switch(instrDecoder.code.opSize) {
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
        registers.statusReg.flags.carry = true;
        registers.statusReg.flags.extend = true;
    } else {
        registers.statusReg.flags.carry = false;
        registers.statusReg.flags.carry = false;
    }
    registers.statusReg.flags.zero = isZero;
    registers.statusReg.flags.negative = (msb)?true:false;
    // Overflow - IF the MSB is toggled during anypoint...
    if (signBefore != MSBForOpSize(instrDecoder.code.opSize, dstReg)) {
        registers.statusReg.flags.overflow = true;
    } else {
        registers.statusReg.flags.overflow = false;
    }
}

// This can be simplified...
// See: MoiraALU_cpp.h - could take a few impl. ideas from there..
void CPUInstructionBase::ExecuteAsrInstr(InstructionDecoder& instrDecoder) {
    auto v = instrDecoder.GetValue();
    if (instrDecoder.opArgDst.addrMode != AddressMode::Register) {
        // raise exception
        return;
    }
    auto &dstReg = GetRegisterValue(instrDecoder.opArgDst.regIndex, instrDecoder.code.opFamily);

    // Fetch the sign-bit..
    uint64_t msb = MSBForOpSize(instrDecoder.code.opSize, dstReg);

    bool carryExtFlag = false;
    bool isZero = false;
    // Loop is needed since we propagate the MSB
    // All this is done 'unsigned' during emulating since signed shift is UB..
    int nBits = v.data.byte;
    while(nBits) {
        carryExtFlag = (dstReg.data.longword & 1)?true:false;
        switch(instrDecoder.code.opSize) {
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
        registers.statusReg.flags.carry = carryExtFlag;
        registers.statusReg.flags.extend = carryExtFlag;
    } else {
        registers.statusReg.flags.carry = false;
    }
    registers.statusReg.flags.zero = isZero;
    registers.statusReg.flags.negative = (msb)?true:false;
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

template<OperandSize szOp, OperandCode opCode>
void Shift(CPUStatusReg &status, int cnt, RegisterValue &regValue) {
    // FIXME: handle all shift operations here (ASR/LSR/ASL/LSL/ROL/ROR)
    if (opCode == OperandCode::LSL) {
        auto v = GetRegVal<szOp>(regValue);
        for(int i=0;i<cnt;i++) {
            v = v << 1;
        }
        SetRegVal<szOp>(regValue, v);
        // FIXME: Update flags
    }
}


// FIXME: Verify and update CPU Status Flags
void CPUInstructionBase::ExecuteLslInstr(InstructionDecoder& instrDecoder) {
    auto v = instrDecoder.GetValue();
    if (instrDecoder.opArgDst.addrMode != AddressMode::Register) {
        // raise exception
        return;
    }
    //auto &dstReg = GetRegisterValue(instrDecoder.dstRegIndex, instrDecoder.opFamily);
    auto dstReg = instrDecoder.ReadDstValue(*this);

    // I would like to get rid of this switch, but I can't without having an encapsulation class
    switch(instrDecoder.code.opSize) {
        case OperandSize::Byte :
            Shift<OperandSize::Byte, OperandCode::LSL>(registers.statusReg, v.data.byte, dstReg);
            break;
        case OperandSize::Word :
            Shift<OperandSize::Word, OperandCode::LSL>(registers.statusReg, v.data.byte, dstReg);
            break;
        case OperandSize::DWord :
            Shift<OperandSize::DWord, OperandCode::LSL>(registers.statusReg, v.data.byte, dstReg);
            break;
        case OperandSize::Long :
            Shift<OperandSize::Long, OperandCode::LSL>(registers.statusReg, v.data.byte, dstReg);
            break;
    }
    WriteToDst(instrDecoder, dstReg);
}

// FIXME: Verify and CPU Status flags
void CPUInstructionBase::ExecuteLsrInstr(InstructionDecoder& instrDecoder) {
    auto v = instrDecoder.GetValue();
    if (instrDecoder.opArgDst.addrMode != AddressMode::Register) {
        // raise exception
        return;
    }
    //auto &dstReg = GetRegisterValue(instrDecoder.dstRegIndex, instrDecoder.opFamily);
    RegisterValue dstReg = instrDecoder.ReadDstValue(*this);

    switch(instrDecoder.code.opSize) {
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
    WriteToDst(instrDecoder, dstReg);
}

//
// Move of these will be small - consider supporting lambda in description code instead...
//
void CPUInstructionBase::ExecuteSysCallInstr(InstructionDecoder& instrDecoder) {
    auto id = registers.dataRegisters[0].data.word;
    if (syscalls.contains(id)) {
        syscalls.at(id)->Invoke(registers, this);
    }
}

void CPUInstructionBase::ExecutePushInstr(InstructionDecoder& instrDecoder) {
    auto &v = instrDecoder.GetValue();
    // FIXME: this should write to register sp using MMU handling
    stack.push(v);
}

void CPUInstructionBase::ExecutePopInstr(InstructionDecoder& instrDecoder) {
    auto v = stack.top();
    stack.pop();
    WriteToDst(instrDecoder, v);
}

void CPUInstructionBase::ExecuteLeaInstr(InstructionDecoder& instrDecoder) {
    auto &v = instrDecoder.GetValue();
    WriteToDst(instrDecoder, v);
}


void CPUInstructionBase::ExecuteMoveInstr(InstructionDecoder& instrDecoder) {
    auto &v = instrDecoder.GetValue();
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


void CPUInstructionBase::ExecuteAddInstr(InstructionDecoder& instrDecoder) {
    auto &v = instrDecoder.GetValue();

    RegisterValue tmpReg = instrDecoder.ReadDstValue(*this);
    switch(instrDecoder.code.opSize) {
        case OperandSize::Byte :
            AddValues<uint8_t>(registers.statusReg, tmpReg, v);
            break;
        case OperandSize::Word :
            AddValues<uint16_t>(registers.statusReg, tmpReg, v);
            break;
        case OperandSize::DWord :
            AddValues<uint32_t>(registers.statusReg, tmpReg, v);
            break;
        case OperandSize::Long :
            AddValues<uint64_t>(registers.statusReg, tmpReg, v);
            break;
    }
    WriteToDst(instrDecoder, tmpReg);
}

void CPUInstructionBase::ExecuteSubInstr(InstructionDecoder& instrDecoder) {
    auto &v = instrDecoder.GetValue();

    RegisterValue tmpReg = instrDecoder.ReadDstValue(*this);
    switch(instrDecoder.code.opSize) {
        case OperandSize::Byte :
            SubtractValues<uint8_t>(registers.statusReg, tmpReg, v);
            break;
        case OperandSize::Word :
            SubtractValues<uint16_t>(registers.statusReg, tmpReg, v);
            break;
        case OperandSize::DWord :
            SubtractValues<uint32_t>(registers.statusReg, tmpReg, v);
            break;
        case OperandSize::Long :
            SubtractValues<uint64_t>(registers.statusReg, tmpReg, v);
            break;
    }
    WriteToDst(instrDecoder, tmpReg);
}

void CPUInstructionBase::ExecuteMulInstr(InstructionDecoder& instrDecoder) {

}
void CPUInstructionBase::ExecuteDivInstr(InstructionDecoder& instrDecoder) {

}

void CPUInstructionBase::ExecuteCallInstr(InstructionDecoder& instrDecoder) {
    auto &v = instrDecoder.GetValue();
    auto retAddr = registers.instrPointer;
    // push on stack...
    stack.push(retAddr);

    switch(instrDecoder.code.opSize) {
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

void CPUInstructionBase::ExecuteRetInstr(InstructionDecoder& instrDecoder) {
    if (stack.empty()) {
        // FIXME: Raise CPU exception!
        fmt::println(stderr, "RET - no return address - stack empty!!");
        return;
    }
    auto newInstrAddr = stack.top();
    stack.pop();
    registers.instrPointer.data = newInstrAddr.data;
}

void CPUInstructionBase::ExecuteRtiInstr(InstructionDecoder& instrDecoder) {
    if (isrControlBlock.isrState != CPUISRState::Executing) {
        // FIXME: Raise invalid CPU state exception
        return;
    }
    // Restore registers, this will restore ALL incl. status and interrupt masks
    // is this what we want?
    registers = isrControlBlock.registersBefore;

    // Reset the ISR State
    isrControlBlock.isrState = CPUISRState::Waiting;
}

//
// Could be moved to base class
//
void CPUInstructionBase::WriteToDst(InstructionDecoder& instrDecoder, const RegisterValue &v) {
    // FIXME: Support more address mode
    if (instrDecoder.opArgDst.addrMode == AddressMode::Register) {
        auto &reg = GetRegisterValue(instrDecoder.opArgDst.regIndex, instrDecoder.code.opFamily);
        reg.data = v.data;
    } else if (instrDecoder.opArgDst.addrMode == AddressMode::Absolute) {
        WriteToMemoryUnit(instrDecoder.code.opSize, instrDecoder.opArgDst.absoluteAddr, v);
    } else if (instrDecoder.opArgDst.addrMode == AddressMode::Indirect) {
        auto &reg = GetRegisterValue(instrDecoder.opArgDst.regIndex, instrDecoder.code.opFamily);
        auto relativeAddrOfs = instrDecoder.ComputeRelativeAddress(*this, instrDecoder.opArgDst.relAddrMode);
        WriteToMemoryUnit(instrDecoder.code.opSize, reg.data.longword + relativeAddrOfs, v);

//        auto v = ReadFromMemoryUnit(instrDecoder.opSize, reg.data.longword + relativeAddrOfs);
//        reg.data = v.data;

    } else {

        fmt::println("Address mode not supported!!!!!");
        exit(1);
    }
}
