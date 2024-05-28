//
// Created by gnilk on 27.03.24.
//
// This implements the instruction execution for instruction-set v1
//

#include "InstructionSetV1Impl.h"
#include "InstructionSetV1Def.h"
#include "InstructionSetV1Decoder.h"

using namespace gnilk;
using namespace gnilk::vcpu;

//
bool InstructionSetV1Impl::ExecuteInstruction(CPUBase &cpu, InstructionDecoderBase &baseDecoder) {

    InstructionSetV1Def::DecoderOutput decoderOutput;
    if (!cpu.GetDispatch().Pop(&decoderOutput, sizeof(decoderOutput))) {
        fmt::println(stderr, "[InstructionSetV1Impl::ExecuteInstruction] Unable to fetch decoder output from dispatcher!");
        return false;
    }




    // FIXME: Remove the decoder
    auto &decoder = dynamic_cast<InstructionSetV1Decoder&>(baseDecoder);

    switch(decoderOutput.operand.opCode) {
        case BRK :
            fmt::println(stderr, "BRK - CPU Halted!");
            cpu.Halt();
            // Enable this
            // pipeline.Flush();
            break;
        case NOP :
            break;
        case SYS :
            ExecuteSysCallInstr(cpu, decoderOutput);
            break;
        case CALL :
            ExecuteCallInstr(cpu, decoderOutput);
            break;
        case LEA :
            ExecuteLeaInstr(cpu, decoderOutput);
            break;
        case RET :
            ExecuteRetInstr(cpu, decoderOutput);
            break;
        case RTI :
            ExecuteRtiInstr(cpu, decoderOutput);
            break;
        case RTE :
            ExecuteRteInstr(cpu, decoderOutput);
            break;
        case MOV :
            ExecuteMoveInstr(cpu, decoderOutput);
            break;
        case ADD :
            ExecuteAddInstr(cpu, decoderOutput);
            break;
        case PUSH :
            ExecutePushInstr(cpu, decoderOutput);
            break;
        case POP :
            ExecutePopInstr(cpu, decoderOutput);
            break;
        case LSR :
            ExecuteLsrInstr(cpu, decoderOutput);
            break;
        case LSL :
            ExecuteLslInstr(cpu, decoderOutput);
            break;
        case ASR :
            ExecuteAsrInstr(cpu, decoderOutput);
            break;
        case ASL :
            ExecuteAslInstr(cpu, decoderOutput);
            break;
        case CMP :
            ExecuteCmpInstr(cpu, decoderOutput);
            break;
        case BEQ :
            ExecuteBeqInstr(cpu, decoderOutput);
            break;
        case BNE :
            ExecuteBneInstr(cpu, decoderOutput);
            break;
        case SIMD:
            ExecuteSIMDInstr(cpu, decoder);
            break;
        default:
            fmt::println(stderr, "Invalid operand: {} - raising exception handler (if available)", decoder.code.opCodeByte);
            //
            if (!cpu.RaiseException(CPUKnownExceptions::kInvalidInstruction)) {
                return false;
            }

    }
    return true;
}


void InstructionSetV1Impl::ExecuteSIMDInstr(CPUBase &cpu, InstructionSetV1Decoder &baseDecoder) {
    // FIXME: This doesn't support assymetric multi-core, i.e. different number of cores depending on the instr. set
    //        In essence there must be an equal amount of cores..

//    auto simdDecoder = instrDecoder.GetDecoderForExtension(SIMD);
//    // Note: This should not happen
//    if (simdDecoder == nullptr) {
//        fmt::println(stderr,"ERR: Decoder for SIMD extension missing");
//        exit(1);
//    }
//
//    auto &simdInstructionSet = InstructionSetManager::Instance().GetExtension(SIMD);
//    simdInstructionSet.GetImplementation().ExecuteInstruction(cpu, *simdDecoder);
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

void InstructionSetV1Impl::ExecuteBneInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {
    if (decoderOutput.opArgDst.addrMode != AddressMode::Immediate) {
        cpu.RaiseException(CPUKnownExceptions::kInvalidAddrMode);
        return;
    }
    // zero must not be set in order to jump
    if (cpu.registers.statusReg.flags.zero == 1) {
        return;
    }

    // FIXME: This is same as for BEQ - we could keep these..

    auto &v = decoderOutput.primaryValue;
    int64_t relativeOffset = 0;

    switch(decoderOutput.operand.opSize) {
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
            cpu.registers.instrPointer.data.longword = v.data.longword;
            return;
    }
    cpu.registers.instrPointer.data.longword += relativeOffset;

}
void InstructionSetV1Impl::ExecuteBeqInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {
    auto &v = decoderOutput.primaryValue;
    if (decoderOutput.opArgDst.addrMode != AddressMode::Immediate) {
        cpu.RaiseException(CPUKnownExceptions::kInvalidAddrMode);
        return;
    }
    // zero must be in order to jump
    if (cpu.registers.statusReg.flags.zero == 0) {
        return;
    }

    int64_t relativeOffset = 0;

    switch(decoderOutput.operand.opSize) {
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
            cpu.registers.instrPointer.data.longword = v.data.longword;
            return;
    }
    cpu.registers.instrPointer.data.longword += relativeOffset;
}

void InstructionSetV1Impl::ExecuteCmpInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {
    auto &v = decoderOutput.primaryValue;
    if (decoderOutput.opArgDst.addrMode == AddressMode::Immediate) {
        cpu.RaiseException(CPUKnownExceptions::kInvalidAddrMode);
        return;
    }

    // FIXME: We shouldn't read the dst value here - should be part of the decoding step...
    //RegisterValue dstReg = instrDecoder.ReadDstValue(cpu);
    RegisterValue &dstReg = decoderOutput.secondaryValue;
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

    switch(decoderOutput.operand.opSize) {
        case OperandSize::Byte :
            UpdateCPUFlagsCMP<uint8_t>(cpu.registers.statusReg, dstReg.data.byte - v.data.byte, dstReg.data.byte, v.data.byte);
            break;
        case OperandSize::Word:
            UpdateCPUFlagsCMP<uint16_t>(cpu.registers.statusReg, dstReg.data.word - v.data.word, dstReg.data.word, v.data.word);
            break;
        case OperandSize::DWord :
            UpdateCPUFlagsCMP<uint32_t>(cpu.registers.statusReg, dstReg.data.dword - v.data.dword, dstReg.data.dword, v.data.dword);
            break;
        case OperandSize::Long :
            UpdateCPUFlagsCMP<uint64_t>(cpu.registers.statusReg, dstReg.data.longword - v.data.longword, dstReg.data.longword, v.data.longword);
            break;
    }
}

void InstructionSetV1Impl::ExecuteAslInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {
    auto &v = decoderOutput.primaryValue;

    if (decoderOutput.opArgDst.addrMode != AddressMode::Register) {
        cpu.RaiseException(CPUKnownExceptions::kInvalidAddrMode);
        return;
    }

    auto &dstReg = cpu.GetRegisterValue(decoderOutput.opArgDst.regIndex, decoderOutput.operand.opFamily);

    auto signBefore = MSBForOpSize(decoderOutput.operand.opSize, dstReg);

    uint64_t msb = 0;
    bool isZero = false;
    int nBits = v.data.byte;
    while(nBits > 0) {
        // Will we shift out something??
        msb = MSBForOpSize(decoderOutput.operand.opSize, dstReg);
        switch(decoderOutput.operand.opSize) {
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
        cpu.registers.statusReg.flags.carry = true;
        cpu.registers.statusReg.flags.extend = true;
    } else {
        cpu.registers.statusReg.flags.carry = false;
        cpu.registers.statusReg.flags.carry = false;
    }
    cpu.registers.statusReg.flags.zero = isZero;
    cpu.registers.statusReg.flags.negative = (msb)?true:false;
    // Overflow - IF the MSB is toggled during anypoint...
    if (signBefore != MSBForOpSize(decoderOutput.operand.opSize, dstReg)) {
        cpu.registers.statusReg.flags.overflow = true;
    } else {
        cpu.registers.statusReg.flags.overflow = false;
    }
}

// This can be simplified...
// See: MoiraALU_cpp.h - could take a few impl. ideas from there..
void InstructionSetV1Impl::ExecuteAsrInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {
    auto &v = decoderOutput.primaryValue;

    if (decoderOutput.opArgDst.addrMode != AddressMode::Register) {
        cpu.RaiseException(CPUKnownExceptions::kInvalidAddrMode);
        return;
    }
    auto &dstReg = cpu.GetRegisterValue(decoderOutput.opArgDst.regIndex, decoderOutput.operand.opFamily);

    // Fetch the sign-bit..
    uint64_t msb = MSBForOpSize(decoderOutput.operand.opSize, dstReg);

    bool carryExtFlag = false;
    bool isZero = false;
    // Loop is needed since we propagate the MSB
    // All this is done 'unsigned' during emulating since signed shift is UB..
    int nBits = v.data.byte;
    while(nBits) {
        carryExtFlag = (dstReg.data.longword & 1)?true:false;
        switch(decoderOutput.operand.opSize) {
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
        cpu.registers.statusReg.flags.carry = carryExtFlag;
        cpu.registers.statusReg.flags.extend = carryExtFlag;
    } else {
        cpu.registers.statusReg.flags.carry = false;
    }
    cpu.registers.statusReg.flags.zero = isZero;
    cpu.registers.statusReg.flags.negative = (msb)?true:false;
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
void InstructionSetV1Impl::ExecuteLslInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {

    auto &v = decoderOutput.primaryValue;
    if (decoderOutput.opArgDst.addrMode != AddressMode::Register) {
        cpu.RaiseException(CPUKnownExceptions::kInvalidAddrMode);
        return;
    }

    auto &dstReg = decoderOutput.secondaryValue;

    // I would like to get rid of this switch, but I can't without having an encapsulation class
    switch(decoderOutput.operand.opSize) {
        case OperandSize::Byte :
            Shift<OperandSize::Byte, OperandCode::LSL>(cpu.registers.statusReg, v.data.byte, dstReg);
            break;
        case OperandSize::Word :
            Shift<OperandSize::Word, OperandCode::LSL>(cpu.registers.statusReg, v.data.byte, dstReg);
            break;
        case OperandSize::DWord :
            Shift<OperandSize::DWord, OperandCode::LSL>(cpu.registers.statusReg, v.data.byte, dstReg);
            break;
        case OperandSize::Long :
            Shift<OperandSize::Long, OperandCode::LSL>(cpu.registers.statusReg, v.data.byte, dstReg);
            break;
    }
    WriteToDst(cpu, decoderOutput, dstReg);
}

// FIXME: Verify and CPU Status flags
void InstructionSetV1Impl::ExecuteLsrInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {
    auto &v = decoderOutput.primaryValue;

    if (decoderOutput.opArgDst.addrMode != AddressMode::Register) {
        cpu.RaiseException(CPUKnownExceptions::kInvalidAddrMode);
        return;
    }
    //auto &dstReg = GetRegisterValue(instrDecoder.dstRegIndex, instrDecoder.opFamily);
    //RegisterValue dstReg = instrDecoder.ReadDstValue(cpu);
    RegisterValue &dstReg = decoderOutput.secondaryValue;

    switch(decoderOutput.operand.opSize) {
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
    WriteToDst(cpu, decoderOutput, dstReg);
}

//
// Move of these will be small - consider supporting lambda in description code instead...
//
void InstructionSetV1Impl::ExecuteSysCallInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {
    auto id = cpu.registers.dataRegisters[0].data.word;
    if (cpu.syscalls.contains(id)) {
        cpu.syscalls.at(id)->Invoke(cpu.registers, &cpu);
    }
}

void InstructionSetV1Impl::ExecutePushInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {
    auto &v = decoderOutput.primaryValue;
    // FIXME: this should write to register sp using MMU handling
    cpu.stack.push(v);
}

void InstructionSetV1Impl::ExecutePopInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {
    auto v = cpu.stack.top();
    cpu.stack.pop();
    WriteToDst(cpu, decoderOutput, v);
}

void InstructionSetV1Impl::ExecuteLeaInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {
    auto &v = decoderOutput.primaryValue;
    WriteToDst(cpu, decoderOutput, v);
}


void InstructionSetV1Impl::ExecuteMoveInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {
    auto &v = decoderOutput.primaryValue;
    WriteToDst(cpu, decoderOutput, v);
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


void InstructionSetV1Impl::ExecuteAddInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {
    auto &v = decoderOutput.primaryValue;


    //RegisterValue tmpReg = instrDecoder.ReadDstValue(cpu);
    RegisterValue tmpReg = decoderOutput.secondaryValue;        // Check if we really want a copy here or a reference
    switch(decoderOutput.operand.opSize) {
        case OperandSize::Byte :
            AddValues<uint8_t>(cpu.registers.statusReg, tmpReg, v);
            break;
        case OperandSize::Word :
            AddValues<uint16_t>(cpu.registers.statusReg, tmpReg, v);
            break;
        case OperandSize::DWord :
            AddValues<uint32_t>(cpu.registers.statusReg, tmpReg, v);
            break;
        case OperandSize::Long :
            AddValues<uint64_t>(cpu.registers.statusReg, tmpReg, v);
            break;
    }
    WriteToDst(cpu, decoderOutput, tmpReg);
}

void InstructionSetV1Impl::ExecuteSubInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {
    auto &v = decoderOutput.primaryValue;


    //RegisterValue tmpReg = instrDecoder.ReadDstValue(cpu);
    RegisterValue tmpReg = decoderOutput.secondaryValue;
    switch(decoderOutput.operand.opSize) {
        case OperandSize::Byte :
            SubtractValues<uint8_t>(cpu.registers.statusReg, tmpReg, v);
            break;
        case OperandSize::Word :
            SubtractValues<uint16_t>(cpu.registers.statusReg, tmpReg, v);
            break;
        case OperandSize::DWord :
            SubtractValues<uint32_t>(cpu.registers.statusReg, tmpReg, v);
            break;
        case OperandSize::Long :
            SubtractValues<uint64_t>(cpu.registers.statusReg, tmpReg, v);
            break;
    }
    WriteToDst(cpu, decoderOutput, tmpReg);
}

void InstructionSetV1Impl::ExecuteMulInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {

}
void InstructionSetV1Impl::ExecuteDivInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {

}

void InstructionSetV1Impl::ExecuteCallInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {
    auto &v = decoderOutput.primaryValue;

    auto retAddr = cpu.registers.instrPointer;
    // push on stack...
    cpu.stack.push(retAddr);

    switch(decoderOutput.operand.opSize) {
        case OperandSize::Byte :
            cpu.registers.instrPointer.data.longword += v.data.byte;
            break;
        case OperandSize::Word :
            cpu.registers.instrPointer.data.longword += v.data.word;
            break;
        case OperandSize::DWord :
            cpu.registers.instrPointer.data.longword += v.data.dword;
            break;
        case OperandSize::Long :
            cpu.registers.instrPointer.data.longword = v.data.longword;     // jumping long-word is absolute!
            break;
    }
}

void InstructionSetV1Impl::ExecuteRetInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {
    if (cpu.stack.empty()) {
        // FIXME: Raise CPU exception!
        fmt::println(stderr, "RET - no return address - stack empty!!");
        cpu.RaiseException(CPUKnownExceptions::kHardFault);
        cpu.Halt();
        return;
    }
    auto newInstrAddr = cpu.stack.top();
    cpu.stack.pop();
    cpu.registers.instrPointer.data = newInstrAddr.data;
}

void InstructionSetV1Impl::ExecuteRtiInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {
    // Check if the CPU is executing the ISR?

    if (!cpu.IsCPUISRActive()) {
        cpu.RaiseException(CPUKnownExceptions::kHardFault);
        return;
    }

    // Active ISR is stored in the CPU status control register...
    auto isrControlBlock = cpu.GetActiveISRControlBlock();

    if (isrControlBlock->isrState != CPUISRState::Executing) {
        // FIXME: HAve specific instruction for this?
        cpu.RaiseException(CPUKnownExceptions::kHardFault);
        return;
    }
    // Restore registers, this will restore ALL incl. status and interrupt masks
    // is this what we want?
    cpu.registers = isrControlBlock->registersBefore;
    // This will reset the state and a few other things
    cpu.ResetActiveISR();
}

void InstructionSetV1Impl::ExecuteRteInstr(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput) {
    if (!cpu.IsCPUExpActive()) {
        // FIXME: Raise invalid CPU state exception
        cpu.RaiseException(CPUKnownExceptions::kHardFault);
        return;
    }
    // Restore registers, this will restore ALL incl. status and interrupt masks
    cpu.registers = cpu.expControlBlock->registersBefore;

    // We are pointing to the instruction causing the illegal instruction - let's point on next..
    cpu.registers.instrPointer.data.longword += 1;

    // Reset the ISR State
    cpu.ResetActiveExp();
}

//
// Could be moved to base class
//
void InstructionSetV1Impl::WriteToDst(CPUBase &cpu, InstructionSetV1Def::DecoderOutput &decoderOutput, const RegisterValue &v) {

    if (decoderOutput.opArgDst.addrMode == AddressMode::Register) {
        auto &reg = cpu.GetRegisterValue(decoderOutput.opArgDst.regIndex, decoderOutput.operand.opFamily);
        reg.data = v.data;
    } else if (decoderOutput.opArgDst.addrMode == AddressMode::Absolute) {
        cpu.WriteToMemoryUnit(decoderOutput.operand.opSize, decoderOutput.opArgDst.absoluteAddr, v);
    } else if (decoderOutput.opArgDst.addrMode == AddressMode::Indirect) {
        auto &reg = cpu.GetRegisterValue(decoderOutput.opArgDst.regIndex, decoderOutput.operand.opFamily);

        // FIXME: This has to be solved in the decoding step...
        //auto relativeAddrOfs = InstructionSetV1Def::ComputeRelativeAddress(cpu, decoderOutput.opArgDst.relAddrMode);
        //cpu.WriteToMemoryUnit(decoderOutput.operand.opSize, reg.data.longword + relativeAddrOfs, v);
        cpu.WriteToMemoryUnit(decoderOutput.operand.opSize, reg.data.longword + decoderOutput.opArgDst.relativeAddressOfs, v);

//        auto v = ReadFromMemoryUnit(instrDecoder.opSize, reg.data.longword + relativeAddrOfs);
//        reg.data = v.data;

    } else {
        cpu.RaiseException(CPUKnownExceptions::kHardFault);
        fmt::println("Address mode not supported!!!!!");
        exit(1);
    }
}

