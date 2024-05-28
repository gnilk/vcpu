//
// Created by gnilk on 16.12.23.
//

//
// This defines the instruction set for v1 - the base instruction set (integer arithmetic)
// It mainly serves as the data definition for data structures used within the decoder / execution part..
//
#include <unordered_map>
#include <optional>
#include "InstructionSetDefBase.h"
#include "InstructionSetV1Def.h"
#include "Simd/SIMDInstructionDecoder.h"
#include "InstructionSetV1/InstructionSetV1Decoder.h"


using namespace gnilk;
using namespace gnilk::vcpu;


//
// This holds the full instruction set definition
// I probably want the features to specifify valid SRC/DST combos
//
static std::unordered_map<OperandCodeBase, OperandDescriptionBase> instructionSet = {
    {OperandCode::SYS,{.name="syscall", .features = {} }},
    {OperandCode::NOP,{.name="nop", .features = {} }},
    {OperandCode::BRK,{.name="brk", .features = {} }},
    {OperandCode::RET,{.name="ret", .features = {} }},
    {OperandCode::RTI,{.name="rti", .features = {} }},
    {OperandCode::RTE,{.name="rte", .features = {} }},
{OperandCode::BEQ,{.name="beq", .features = OperandFeatureFlags::kFeature_OperandSize | OperandFeatureFlags::kFeature_OneOperand | OperandFeatureFlags::kFeature_Immediate | OperandFeatureFlags::kFeature_Branching}},
{OperandCode::BNE,{.name="bne", .features = OperandFeatureFlags::kFeature_OperandSize | OperandFeatureFlags::kFeature_OneOperand | OperandFeatureFlags::kFeature_Immediate | OperandFeatureFlags::kFeature_Branching}},

{OperandCode::LSL, {.name="lsl", .features = OperandFeatureFlags::kFeature_OperandSize |
                                             OperandFeatureFlags::kFeature_TwoOperands |
                                             OperandFeatureFlags::kFeature_Immediate |
                                             OperandFeatureFlags::kFeature_TwoOpReadSecondary |
                                             OperandFeatureFlags::kFeature_AnyRegister }},
{OperandCode::LSR, {.name="lsr", .features = OperandFeatureFlags::kFeature_OperandSize |
                                             OperandFeatureFlags::kFeature_TwoOperands |
                                             OperandFeatureFlags::kFeature_Immediate |
                                             OperandFeatureFlags::kFeature_TwoOpReadSecondary |
                                             OperandFeatureFlags::kFeature_AnyRegister }},
{OperandCode::ASL, {.name="asl", .features = OperandFeatureFlags::kFeature_OperandSize |
                                             OperandFeatureFlags::kFeature_TwoOperands |
                                             OperandFeatureFlags::kFeature_Immediate |
                                             OperandFeatureFlags::kFeature_AnyRegister }},
{OperandCode::ASR, {.name="asr", .features = OperandFeatureFlags::kFeature_OperandSize |
                                             OperandFeatureFlags::kFeature_TwoOperands |
                                             OperandFeatureFlags::kFeature_Immediate |
                                             OperandFeatureFlags::kFeature_AnyRegister }},
{OperandCode::LEA,{.name="lea", .features = OperandFeatureFlags::kFeature_OperandSize |
                                            OperandFeatureFlags::kFeature_TwoOperands |
                                            OperandFeatureFlags::kFeature_Immediate |
                                            OperandFeatureFlags::kFeature_AnyRegister |
                                            OperandFeatureFlags::kFeature_Addressing |
                                            OperandFeatureFlags::kFeature_Branching}},       // <- FIXME: Rename this
{OperandCode::MOV,{.name="move", .features = OperandFeatureFlags::kFeature_OperandSize |
                                             OperandFeatureFlags::kFeature_TwoOperands |
                                             OperandFeatureFlags::kFeature_Immediate |
                                             OperandFeatureFlags::kFeature_Control |
                                             OperandFeatureFlags::kFeature_AnyRegister |
                                             OperandFeatureFlags::kFeature_Addressing}},
    {OperandCode::CMP, {.name="cmp", .features = OperandFeatureFlags::kFeature_OperandSize |
                                                 OperandFeatureFlags::kFeature_TwoOperands |
                                                 OperandFeatureFlags::kFeature_Immediate |
                                                 OperandFeatureFlags::kFeature_AnyRegister |
                                                 OperandFeatureFlags::kFeature_Addressing |
                                                 OperandFeatureFlags::kFeature_TwoOpReadSecondary }},
    {OperandCode::ADD,{.name="add", .features = OperandFeatureFlags::kFeature_OperandSize |
                                                OperandFeatureFlags::kFeature_TwoOperands |
                                                OperandFeatureFlags::kFeature_Immediate |
                                                OperandFeatureFlags::kFeature_TwoOpReadSecondary |
                                                OperandFeatureFlags::kFeature_AnyRegister |
                                                OperandFeatureFlags::kFeature_Addressing}},
{OperandCode::SUB,{.name="sub", .features = OperandFeatureFlags::kFeature_OperandSize |
                                            OperandFeatureFlags::kFeature_TwoOperands |
                                            OperandFeatureFlags::kFeature_Immediate |
                                            OperandFeatureFlags::kFeature_TwoOpReadSecondary |
                                            OperandFeatureFlags::kFeature_AnyRegister |
                                            OperandFeatureFlags::kFeature_Addressing}},
{OperandCode::MUL,{.name="mul", .features = OperandFeatureFlags::kFeature_OperandSize |
                                            OperandFeatureFlags::kFeature_TwoOperands |
                                            OperandFeatureFlags::kFeature_Immediate |
                                            OperandFeatureFlags::kFeature_AnyRegister |
                                            OperandFeatureFlags::kFeature_Addressing}},
{OperandCode::DIV,{.name="div", .features = OperandFeatureFlags::kFeature_OperandSize |
                                            OperandFeatureFlags::kFeature_TwoOperands |
                                            OperandFeatureFlags::kFeature_Immediate |
                                            OperandFeatureFlags::kFeature_AnyRegister |
                                            OperandFeatureFlags::kFeature_Addressing}},

    // Push can be from many sources
  {OperandCode::PUSH,{.name="push", .features = OperandFeatureFlags::kFeature_OperandSize | OperandFeatureFlags::kFeature_OneOperand | OperandFeatureFlags::kFeature_AnyRegister | OperandFeatureFlags::kFeature_Immediate | OperandFeatureFlags::kFeature_Addressing}},
    // Pop can only be to register...
  {OperandCode::POP,{.name="pop", .features = OperandFeatureFlags::kFeature_OperandSize | OperandFeatureFlags::kFeature_OneOperand | OperandFeatureFlags::kFeature_AnyRegister}},

    {OperandCode::CALL, {.name="call", .features = OperandFeatureFlags::kFeature_Branching | OperandFeatureFlags::kFeature_OperandSize | OperandFeatureFlags::kFeature_OneOperand | OperandFeatureFlags::kFeature_Immediate | OperandFeatureFlags::kFeature_AnyRegister | OperandFeatureFlags::kFeature_Addressing}},
    // Extension byte - nothing, no name - not available to the assembler
    {OperandCode::SIMD, {.name="", .features = OperandFeatureFlags::kFeature_Extension }},
};

const std::unordered_map<OperandCodeBase, OperandDescriptionBase> &InstructionSetV1Def::GetInstructionSet() {
    return instructionSet;
}


std::optional<OperandDescriptionBase> InstructionSetV1Def::GetOpDescFromClass(OperandCodeBase opClass) {
    if (!instructionSet.contains(opClass)) {
        return{};
    }
    return instructionSet.at(opClass);
}

// Look up table is built when an operand from str is first called
static std::unordered_map<std::string, OperandCodeBase> strToOpClassMap;

std::optional<OperandCodeBase> InstructionSetV1Def::GetOperandFromStr(const std::string &str) {
    // Lazy create on first request
    static bool haveNameOpClassTable = false;
    if (!haveNameOpClassTable) {
        for(auto [opClass, desc] : instructionSet) {
            strToOpClassMap[desc.name] = opClass;
        }
        haveNameOpClassTable = true;
    }

    if (!strToOpClassMap.contains(str)) {
        return {};
    }
    return strToOpClassMap.at(str);
}

// Helper to encode this according to spec - called from assembler StateEmitter when generating binary code..
//uint8_t InstructionSetV1Def::EncodeOpSizeAndFamily(OperandSize opSize, OperandFamily opFamily) {
//    uint8_t opSizeAndFamilyCode = opSize;
//    opSizeAndFamilyCode |= static_cast<uint8_t>(opFamily) << 4;
//    return opSizeAndFamilyCode;
//}


