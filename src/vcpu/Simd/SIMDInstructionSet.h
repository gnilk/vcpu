//
// Created by gnilk on 16.05.24.
//

#ifndef VCPU_SIMDINSTRUCTIONSET_H
#define VCPU_SIMDINSTRUCTIONSET_H

#include <stdint.h>
#include <string>
#include <unordered_map>
#include <type_traits>

#include "BitmaskFlagOperators.h"

namespace gnilk {
    namespace vcpu {
        // This is the SIMD/CU instruction set extension for VCPU
        // One can use this 'as-is' with a RAW instruction stream or through the generic CPU InstructionBase
        // which will then be mapped through one of the 15 extensions (0xf0..0xfe)

        //
        // Encoding
        //  - Each instruction is fixed 32 bits...
        //  - No conditional status flags <- or????
        //  - Special register for 'zero' ?
        //
        // - How many instructions do we need???
        //   This would reserve 64 instructions
        // RRCC CCCC | FFFF | DDDD | AAAA | BBBB |
        //
        // How do we solve matrix multiplication?
        //  - 4x4 is fairly easy with dp4, but still - one matrix needs to be transposed in order for that to work..
        //  - how to "hold" -> or "point" to a matrix?
        //    too slow to reference RAM?
        //
        // aPtr, aStride
        // bPtr, bStride,
        // dPtr, dStride,
        //
        //
        //  RR - reserved
        //  CC CCCC - Op Code
        //  FFFF - Flags
        //  DDDD - Destination Register
        //  AAAA - Operand Register A
        //  BBBB - Operand Register B
        //
        //
        // Example:
        //      dp3  r0, r1, r2         // perform vec3 dot product like; r0 = r1 DOT r2  - DO NOT update conditional register
        //      dp3  r3,r1,r2           // r3 = r1 DOT r2
        //      ble  rz,r1,<offset>     // max distance is 64 bytes (16 * 4)
        //
        // BLE encoding
        //   00CC CCCC | rz | r1 | <ofs>    // offset will be shifted 4 - max 16 instructions forward or 64 bytes...
        //
        //
        // Flags:
        //      F/I - float or int, 1 = float, 0 = int
        //      C   - update condition
        //



        //
        // instructions:
        // This is straight of taken from NVidia GPU v4
        // http://www.renderguild.com/gpuguide.pdf
        //
        enum class SimdOpCode : uint8_t {
            ABS,            // absolute value
            ADD,            // add
            AND,            // btwise and
            BRK,            // break out of loop
            CALL,           // call sub routine
            CEIL,           // ceiling
            CMP,            // compare
            CONT,           // continue loop
            COS,            // cosine with reduction (-pi,pi)
            DIV,            // divide
            DP2,            // 2-component dot product
            DP2A,           // 2-component dot product w/scalar add
            DP3,            // 3-component dot product
            DP4,            // 4-component dot product
            DPH,            // homogeneous dot product
            DST,            // distance vector
            EX2,            // exponential base 2
            FLR,            // floor
            FRC,            // fraction
            I2F,            // int 2 float
            LG2,            // lograithm base 2
            LRP,            // lerp - linerar interpolation
            MAD,            // multiply and add
            MAX,            // max
            MIN,            // min
            MOD,            // modulus per component by scalar
            MOV,            // move
            MUL,            // multiply
            NOT,            // bitwise not
            NRM,            // normalize 3 component vector
            OR,             // bitwise or
            PK2H,           // pack two 16bit floats
            PK2US,          // pack two floats as unsigned 16 bit
            PK4B,           // pack four float as signed 8 bit
            PK4UB,          // pack four floats as unsigned 8 bit
            POW,            // exponentiate
            RCC,            // reciprocal (clamped)
            RCP,            // reciprocal
            REP,            // repeat
            RET,            // return
            ROUND,          // round to nearest integer
            RSQ,            // reciprocal square root
            SAD,            // sum of absolute differences
            SCS,            // sin/cos without reduction
            SHL,            // bit shift left
            SHR,            // bit shift right
            SIN,            // sin with reduction (-pi,pi)
            SUB,            // subtract
            SMP,            // sample
            SWZ,            // extended swizzle
            TRUNC,          // truncate (round towards zero)
            UP2H,           // unpack two 16 bit floats
            UP2US,          // unpack two unsigned 16-bit ints
            UP4B,           // unpack four 8-bit ints
            UP4UB,          // unpack four 8-bit unsigned ints
            XOR,            // bitwise exclusive or
            XPD,            // cross product
        };

        typedef enum : uint8_t {
            kFloat,
            kSigned,
            kConditionCode,
        } SimdOpCodeFlags;

        enum class SimdOpCodeFeatureFlags : uint16_t {
            kFeature_Float = 0x01,
            kFeature_Integer = 0x02,
            kFeature_Condition = 0x04,
            kFeature_Clamping = 0x08,
            kFeature_HalfPrecision = 0x10,
            kFeature_Def_Float = 0x20,
            kFeature_Def_Signed = 0x40,
            kFeature_Def_Unsigned = 0x80,
        };


        template<>
        struct EnableBitmaskOperators<SimdOpCodeFeatureFlags> {
            static const bool enable = true;
        };


        struct SimdOperandDescription {
            std::string name;
            SimdOpCodeFeatureFlags  features;
        };


        class SIMDInstructionSet {
        public:
            static std::unordered_map<SimdOpCode, SimdOperandDescription> &GetInstructionSet();
        private:
            static std::unordered_map<SimdOpCode, SimdOperandDescription> instructionSet;
        };
    }
}


#endif //VCPU_SIMDINSTRUCTIONSET_H
