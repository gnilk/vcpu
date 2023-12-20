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

