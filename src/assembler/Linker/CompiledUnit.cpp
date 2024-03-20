//
// Created by gnilk on 20.12.23.
//
#include "fmt/format.h"

#include <memory>
#include <unordered_map>
#include "Linker/CompiledUnit.h"
#include "Compiler/Context.h"

using namespace gnilk;
using namespace gnilk::assembler;


// Compiled Unit
void CompiledUnit::Clear() {
    // Nothing to do?
}


size_t CompiledUnit::Write(Context &context, const std::vector<uint8_t> &data) {
    if (context.GetActiveSegment() == nullptr) {
        return 0;
    }
    auto segment = context.GetActiveSegment();
    if (segment->CurrentChunk() == nullptr) {
        return 0;
    }
    auto nWritten = segment->CurrentChunk()->Write(data);

    return nWritten;
}
/*
bool CompiledUnit::WriteByte(Context &context, uint8_t byte) {
    if (context.GetActiveSegment() == nullptr) {
        return false;
    }
    if (!context.GetActiveSegment()->WriteByte(byte)) {
        return false;
    }
    return true;
}
void CompiledUnit::ReplaceAt(Context &context, uint64_t offset, uint64_t newValue) {
    if (context.GetActiveSegment() == nullptr) {
        return;
    }
    context.GetActiveSegment()->ReplaceAt(offset, newValue);
}
void CompiledUnit::ReplaceAt(Context &context, uint64_t offset, uint64_t newValue, vcpu::OperandSize opSize) {
    if (context.GetActiveSegment() == nullptr) {
        return;
    }
    context.GetActiveSegment()->ReplaceAt(offset, newValue, opSize);
}
*/


uint64_t CompiledUnit::GetCurrentWriteAddress(Context &context) {
    if (context.GetActiveSegment() == nullptr) {
        fmt::println(stderr, "Compiler, not active segment!!");
        return 0;
    }
    if (context.GetActiveSegment()->CurrentChunk()) {
        fmt::println(stderr, "Compiler, no chunk in active segment {}", context.GetActiveSegment()->Name());
        return 0;
    }
    return context.GetActiveSegment()->CurrentChunk()->GetCurrentWriteAddress();
}

const std::vector<uint8_t> &CompiledUnit::Data(Context &context) {
    if (!context.GetOrAddSegment("link_out", 0x00)) {
        static std::vector<uint8_t> empty = {};
        return empty;
    }
    return context.GetActiveSegment()->CurrentChunk()->Data();
}
