cmake_minimum_required(VERSION 3.22)
project(vcpu)
set(CMAKE_CXX_STANDARD 20)

list(APPEND fmt_src src/ext/fmt/format.cc)

# note: name is not fmt because it will clash with other projects importing this library
add_library(log_fmt STATIC ${fmt_src})
target_compile_options(log_fmt PUBLIC -fPIC)
target_include_directories(log_fmt PUBLIC src/ext)


if (NOT EXISTS ${CMAKE_SOURCE_DIR}/src/ext)
    message(STATUS "ext directory (for dependencies) missing, creating...")
    execute_process(COMMAND mkdir src/ext)
endif()

if (NOT EXISTS ${CMAKE_SOURCE_DIR}/src/ext/ELFIO)
    message(WARNING "Installing ELFIO library")
    execute_process(COMMAND git clone https://github.com/serge1/ELFIO ${CMAKE_SOURCE_DIR}/src/ext/ELFIO)
endif ()



#
# Virtual CPU sources
#
list(APPEND vcpusrc src/vcpu/CPUBase.cpp src/vcpu/CPUBase.h)
list(APPEND vcpusrc src/vcpu/InstructionSetImpl.cpp src/vcpu/InstructionSetImpl.cpp)
list(APPEND vcpusrc src/vcpu/InstructionDecoder.cpp src/vcpu/InstructionDecoder.h)
list(APPEND vcpusrc src/vcpu/InstructionSet.cpp src/vcpu/InstructionSet.h)
list(APPEND vcpusrc src/vcpu/Interrupt.h)
list(APPEND vcpusrc src/vcpu/Peripheral.h)
list(APPEND vcpusrc src/vcpu/RegisterValue.h)
list(APPEND vcpusrc src/vcpu/System.cpp src/vcpu/System.h)
list(APPEND vcpusrc src/vcpu/SuperScalarCPU.cpp)
list(APPEND vcpusrc src/vcpu/Timer.cpp src/vcpu/Timer.h)
list(APPEND vcpusrc src/vcpu/VirtualCPU.cpp src/vcpu/VirtualCPU.h)

# HW emulated memory handling
list(APPEND vcpusrc src/vcpu/MemorySubSys/Cache.cpp src/vcpu/MemorySubSys/Cache.h)
list(APPEND vcpusrc src/vcpu/MemorySubSys/CacheController.cpp src/vcpu/MemorySubSys/CacheController.h)
list(APPEND vcpusrc src/vcpu/MemorySubSys/MemoryUnit.cpp src/vcpu/MemorySubSys/MemoryUnit.h)
# memory bus(-es)
list(APPEND vcpusrc src/vcpu/MemorySubSys/BusBase.h)
list(APPEND vcpusrc src/vcpu/MemorySubSys/MesiBusBase.cpp src/vcpu/MemorySubSys/MesiBusBase.h)
list(APPEND vcpusrc src/vcpu/MemorySubSys/FlashBus.cpp src/vcpu/MemorySubSys/FlashBus.h)
list(APPEND vcpusrc src/vcpu/MemorySubSys/HWMappedBus.cpp src/vcpu/MemorySubSys/HWMappedBus.h)
list(APPEND vcpusrc src/vcpu/MemorySubSys/RamBus.cpp src/vcpu/MemorySubSys/RamBus.h)
# temp...
list(APPEND vcpusrc src/vcpu/MemorySubSys/PageAllocator.cpp src/vcpu/MemorySubSys/PageAllocator.h)
#
# Virtual CPU unit test sources
#
list(APPEND vcputestsrc src/vcpu/tests/test_integration.cpp)
list(APPEND vcputestsrc src/vcpu/tests/test_interrupt.cpp)
list(APPEND vcputestsrc src/vcpu/tests/test_pipeline.cpp)
list(APPEND vcputestsrc src/vcpu/tests/test_soc.cpp)
list(APPEND vcputestsrc src/vcpu/tests/test_timer.cpp)
list(APPEND vcputestsrc src/vcpu/tests/test_vcpu.cpp)
# mem subsys tests
# list(APPEND vcputestsrc src/vcpu/MemorySubSys/Tests/test_mmu.cpp)
list(APPEND vcputestsrc src/vcpu/MemorySubSys/Tests/test_cache.cpp)
list(APPEND vcputestsrc src/vcpu/MemorySubSys/Tests/test_mmu_new.cpp)



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
list(APPEND asmsrc src/assembler/Compiler/Context.cpp src/assembler/Compiler/Context.h)
#list(APPEND asmsrc src/assembler/Compiler/EmitStatement.cpp src/assembler/Compiler/EmitStatement.h)
list(APPEND asmsrc src/assembler/Compiler/Identifiers.h)
list(APPEND asmsrc src/assembler/Compiler/StmtEmitter.cpp src/assembler/Compiler/StmtEmitter.h)
list(APPEND asmsrc src/assembler/Lexer/TextLexer.cpp src/assembler/Lexer/TextLexer.h)
list(APPEND asmsrc src/assembler/Parser/Parser.cpp src/assembler/Parser/Parser.h)


list(APPEND asmsrc src/assembler/Linker/BaseLinker.cpp src/assembler/Linker/BaseLinker.h)
list(APPEND asmsrc src/assembler/Compiler/CompileUnit.cpp src/assembler/Compiler/CompileUnit.h)
#list(APPEND asmsrc src/assembler/Linker/DummyLinker.cpp src/assembler/Linker/DummyLinker.h)
list(APPEND asmsrc src/assembler/Linker/ElfLinker.cpp src/assembler/Linker/ElfLinker.h)
list(APPEND asmsrc src/assembler/Linker/RawLinker.cpp src/assembler/Linker/RawLinker.h)
list(APPEND asmsrc src/assembler/Linker/Segment.cpp src/assembler/Linker/Segment.h)

list(APPEND commonsrc src/common/ConvertUTF.cpp src/common/ConvertUTF.h)
list(APPEND commonsrc src/common/DurationTimer.cpp src/common/DurationTimer.h)
list(APPEND commonsrc src/common/HexDump.cpp src/common/HexDump.h)
list(APPEND commonsrc src/common/StrUtil.cpp src/common/StrUtil.h)
list(APPEND commonsrc src/common/UnicodeHelper.cpp src/common/UnicodeHelper.h)

#
# Assembler unit test src
#
list(APPEND asmtestsrc src/assembler/tests/test_compiler.cpp)
list(APPEND asmtestsrc src/assembler/tests/test_context.cpp)
list(APPEND asmtestsrc src/assembler/tests/test_lexer.cpp)
list(APPEND asmtestsrc src/assembler/tests/test_linker.cpp)
list(APPEND asmtestsrc src/assembler/tests/test_parser.cpp)
list(APPEND asmtestsrc src/assembler/tests/test_segment.cpp)
list(APPEND asmtestsrc src/assembler/tests/test_stmtemitter.cpp)


# test library for vcpu
add_library(test SHARED ${vcputestsrc} ${asmtestsrc} ${asmsrc} ${vcpusrc} ${commonsrc})
target_compile_options(test PUBLIC -DTRUN_USE_V1)
target_include_directories(test PUBLIC src/vcpu)
target_include_directories(test PUBLIC src/assembler)
target_include_directories(test PUBLIC src/common)
target_include_directories(test PUBLIC src/ext/ELFIO)

if (APPLE)
    target_include_directories(test PUBLIC /usr/local/include)
endif()


#
# Emulator and Assembler apps
#
add_executable(emu apps/emulator/emulator.cpp ${emusrc} ${vcpusrc} ${commonsrc})
target_include_directories(emu PUBLIC src/vcpu)
target_include_directories(emu PUBLIC src/common)
target_include_directories(emu PUBLIC src/ext/ELFIO)

add_executable(asm apps/assembler/assembler.cpp ${asmsrc} ${vcpusrc} ${commonsrc})
target_include_directories(asm PUBLIC src/assembler)
target_include_directories(asm PUBLIC src/common)
target_include_directories(asm PUBLIC src/vcpu)
target_include_directories(asm PUBLIC src/ext/ELFIO)

#
# link targets
#
target_link_libraries(test log_fmt)
target_link_libraries(emu log_fmt)
target_link_libraries(asm log_fmt)

#
# standalone tests
#
#add_executable(test_linker_relocate apps/tests/linker_relocate_otherseg_withorg.cpp ${asmsrc} ${vcpusrc} ${commonsrc})
#target_include_directories(test_linker_relocate PUBLIC src/vcpu)
#target_include_directories(test_linker_relocate PUBLIC src/assembler)
#target_include_directories(test_linker_relocate PUBLIC src/common)
#target_include_directories(test_linker_relocate PUBLIC src/ext/ELFIO)
#target_link_libraries(test_linker_relocate log_fmt)

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
