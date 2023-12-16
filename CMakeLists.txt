cmake_minimum_required(VERSION 3.27)
project(vcpu)
set(CMAKE_CXX_STANDARD 20)

list(APPEND fmt_src src/ext/fmt/format.cc)

# note: name is not fmt because it will clash with other projects importing this library
add_library(log_fmt STATIC ${fmt_src})
target_compile_options(log_fmt PUBLIC -fPIC)
target_include_directories(log_fmt PUBLIC src/ext)

#
# Virtual CPU sources
#
list(APPEND vcpusrc src/vcpu/CPUBase.cpp src/vcpu/CPUBase.h)
list(APPEND vcpusrc src/vcpu/InstructionDecoder.cpp src/vcpu/InstructionDecoder.h)
list(APPEND vcpusrc src/vcpu/InstructionSet.cpp src/vcpu/InstructionSet.h)
list(APPEND vcpusrc src/vcpu/VirtualCPU.cpp src/vcpu/VirtualCPU.h)
#
# Virtual CPU unit test sources
#
list(APPEND vcputestsrc tests/test_vcpu.cpp)


#
# Assembler sources
#
list(APPEND asmsrc src/assembler/ast/ast.cpp src/assembler/ast/ast.h)
list(APPEND asmsrc src/assembler/ast/astbase.h)
list(APPEND asmsrc src/assembler/ast/instructions.h)
list(APPEND asmsrc src/assembler/ast/literals.h)

list(APPEND asmsrc src/assembler/Compiler/Compiler.cpp src/assembler/Compiler/Compiler.h)
list(APPEND asmsrc src/assembler/Lexer/TextLexer.cpp src/assembler/Lexer/TextLexer.h)
list(APPEND asmsrc src/assembler/Parser/Parser.cpp src/assembler/Parser/Parser.h)

list(APPEND asmsrc src/assembler/ConvertUTF.cpp src/assembler/ConvertUTF.h)
list(APPEND asmsrc src/assembler/StrUtil.cpp src/assembler/StrUtil.h)
list(APPEND asmsrc src/assembler/UnicodeHelper.cpp src/assembler/UnicodeHelper.h)

#
# Assembler unit test src
#
list(APPEND asmtestsrc src/assembler/tests/test_lexer.cpp)
list(APPEND asmtestsrc src/assembler/tests/test_parser.cpp)
list(APPEND asmtestsrc src/assembler/tests/test_compiler.cpp)


# test library for vcpu
add_library(emutest SHARED ${vcputestsrc}  ${vcpusrc})
target_include_directories(emutest PUBLIC src/vcpu)

add_library(asmtest SHARED ${asmtestsrc}  ${asmsrc})
target_include_directories(asmtest PUBLIC src/assembler)
target_include_directories(asmtest PUBLIC src/vcpu)


if (APPLE)
    target_include_directories(emutest PUBLIC /usr/local/include)
    target_include_directories(asmtest PUBLIC /usr/local/include)
endif()


#
# Emulator
#
list(APPEND emusrc apps/emulator/emulator.cpp)
list(APPEND asmsrc apps/assembler/assembler.cpp)


add_executable(emulator ${emusrc} ${vcpusrc})
target_include_directories(emulator PUBLIC src/vcpu)

add_executable(asm ${asmsrc})
target_include_directories(asm PUBLIC src/assembler)
target_include_directories(asm PUBLIC src/vcpu)

#
# link targets
#
target_link_libraries(emutest log_fmt)
target_link_libraries(asmtest log_fmt)
target_link_libraries(emulator log_fmt)
target_link_libraries(asm log_fmt)
