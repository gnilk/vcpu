//
// Created by gnilk on 24.05.24.
//
#include <stdint.h>
#include <vector>
#include <testinterface.h>

#include "VirtualCPU.h"
#include "Simd/SIMDInstructionDecoder.h"

using namespace gnilk;
using namespace gnilk::vcpu;

extern "C" {
    DLL_EXPORT int test_simd(ITesting *t);
    DLL_EXPORT int test_simd_decode(ITesting *t);
    DLL_EXPORT int test_simd_decode_ext(ITesting *t);
}
DLL_EXPORT int test_simd(ITesting *t) {
    return kTR_Pass;
}

static uint8_t ram[1024];
DLL_EXPORT int test_simd_decode(ITesting *t) {
    VirtualCPU vcpu;
    SIMDInstructionDecoder simdDecoder;

    uint8_t code[]={
            SimdOpCode::LOAD,
            kSimdOpFlags::kOpFlag_SzFp32 | kSimdOpFlags::kOpFlag_SrcAddrReg,
            0x00,   // flags | dst
            0x00,   // src A | src B
    };
    memcpy(ram, code, sizeof(code));

    vcpu.QuickStart(ram, 1024);
    simdDecoder.Tick(vcpu);


    return kTR_Pass;
}

DLL_EXPORT int test_simd_decode_ext(ITesting *t) {
    VirtualCPU vcpu;
    //SIMDInstructionDecoder simdDecoder;

    uint8_t code[]={
            OperandCode::SIMD,      // extension byte
            SimdOpCode::LOAD,
            kSimdOpFlags::kOpFlag_SzFp32 | kSimdOpFlags::kOpFlag_SrcAddrReg,
            0x00,   // flags | dst
            0x00,   // src A | src B
    };
    memcpy(ram, code, sizeof(code));

    vcpu.QuickStart(ram, 1024);
    //simdDecoder.Tick(vcpu);
    vcpu.Step();


    return kTR_Pass;
}
