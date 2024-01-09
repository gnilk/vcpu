cmake_minimum_required(VERSION 3.22)
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
list(APPEND vcpusrc src/vcpu/MemoryUnit.cpp src/vcpu/MemoryUnit.h)
list(APPEND vcpusrc src/vcpu/VirtualCPU.cpp src/vcpu/VirtualCPU.h)
#
# Virtual CPU unit test sources
#
list(APPEND vcputestsrc tests/test_vcpu.cpp)
list(APPEND vcputestsrc tests/test_integration.cpp)
list(APPEND vcputestsrc tests/test_mmu.cpp)


#
# Assembler sources
#
list(APPEND asmsrc src/assembler/ast/ast.cpp src/assembler/ast/ast.h)
list(APPEND asmsrc src/assembler/ast/astbase.h)
list(APPEND asmsrc src/assembler/ast/instructions.h)
list(APPEND asmsrc src/assembler/ast/literals.h)
list(APPEND asmsrc src/assembler/ast/statements.h)
list(APPEND asmsrc src/assembler/ast/expressions.h)

list(APPEND asmsrc src/assembler/Compiler/Compiler.cpp src/assembler/Compiler/Compiler.h)
list(APPEND asmsrc src/assembler/Lexer/TextLexer.cpp src/assembler/Lexer/TextLexer.h)
list(APPEND asmsrc src/assembler/Parser/Parser.cpp src/assembler/Parser/Parser.h)

list(APPEND asmsrc src/assembler/Linker/CompiledUnit.cpp src/assembler/Linker/CompiledUnit.h)
list(APPEND asmsrc src/assembler/Linker/Segment.h)

list(APPEND commonsrc src/common/ConvertUTF.cpp src/common/ConvertUTF.h)
list(APPEND commonsrc src/common/StrUtil.cpp src/common/StrUtil.h)
list(APPEND commonsrc src/common/UnicodeHelper.cpp src/common/UnicodeHelper.h)

#
# Assembler unit test src
#
list(APPEND asmtestsrc src/assembler/tests/test_lexer.cpp)
list(APPEND asmtestsrc src/assembler/tests/test_parser.cpp)
list(APPEND asmtestsrc src/assembler/tests/test_compiler.cpp)


# test library for vcpu
add_library(test SHARED ${vcputestsrc} ${asmtestsrc} ${asmsrc} ${vcpusrc} ${commonsrc})
target_include_directories(test PUBLIC src/vcpu)
target_include_directories(test PUBLIC src/assembler)
target_include_directories(test PUBLIC src/common)

if (APPLE)
    target_include_directories(test PUBLIC /usr/local/include)
endif()


#
# Emulator
#
list(APPEND emusrc apps/emulator/emulator.cpp)
list(APPEND asmsrc apps/assembler/assembler.cpp)


add_executable(emu ${emusrc} ${vcpusrc} ${commonsrc})
target_include_directories(emu PUBLIC src/vcpu)
target_include_directories(emu PUBLIC src/common)

add_executable(asm ${asmsrc} ${vcpusrc} ${commonsrc})
target_include_directories(asm PUBLIC src/assembler)
target_include_directories(asm PUBLIC src/common)
target_include_directories(asm PUBLIC src/vcpu)

#
# link targets
#
target_link_libraries(test log_fmt)
target_link_libraries(emu log_fmt)
target_link_libraries(asm log_fmt)


#
# assets for testing
#
if (${CMAKE_VERSION} VERSION_LESS "3.26.0")
    add_custom_target(Assets ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/Assets ${CMAKE_CURRENT_BINARY_DIR}/Assets)
else()
    add_custom_target(Assets ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/Assets ${CMAKE_CURRENT_BINARY_DIR}/Assets)
endif()
add_dependencies(asm Assets)
add_dependencies(emu Assets)
add_dependencies(test Assets)
