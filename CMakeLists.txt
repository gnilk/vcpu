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
list(APPEND vcpusrc src/VirtualCPU.cpp src/VirtualCPU.h)

#
# Unit test sources
#
list(APPEND testsrc tests/test_vcpu.cpp)


# test library
add_library(vcpu_test SHARED ${testsrc} ${vcpusrc})
target_include_directories(vcpu_test PUBLIC src)

if (APPLE)
    target_include_directories(vcpu_test PUBLIC /usr/local/include)
endif()


#
# Emulator
#
list(APPEND appsrc main.cpp)


add_executable(emulator ${appsrc} ${vcpusrc})
target_include_directories(emulator PUBLIC src)

#
# link targets
#
target_link_libraries(vcpu_test log_fmt)
target_link_libraries(emulator log_fmt)
