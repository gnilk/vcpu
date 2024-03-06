//
// Created by gnilk on 06.03.24.
//

#include <stdio.h>
#include <filesystem>

#include <testinterface.h>
#include "Parser/Parser.h"
#include "Linker/Segment.h"

using namespace gnilk::assembler;

extern "C" {
    DLL_EXPORT int test_segment(ITesting *t);
    DLL_EXPORT int test_segment_chunksort(ITesting *t);
    DLL_EXPORT int test_segment_chunkfromaddr(ITesting *t);
}
DLL_EXPORT int test_segment(ITesting *t) {
    return kTR_Pass;
}
DLL_EXPORT int test_segment_chunksort(ITesting *t) {
    auto segment = std::make_shared<Segment>("name", 0);
    int byte = 0x00;
    // Write 16 bytes in to all chunks...
    for(int i=0;i<16;i++, byte++) segment->WriteByte(byte);
    segment->CreateChunk(100);
    for(int i=0;i<16;i++, byte++) segment->WriteByte(byte);
    segment->CreateChunk(200);
    for(int i=0;i<16;i++, byte++) segment->WriteByte(byte);
    segment->CreateChunk(300);
    for(int i=0;i<16;i++, byte++) segment->WriteByte(byte);


    auto &chunks = segment->DataChunks();
    for(int i=1;i<chunks.size();i++) {
        TR_ASSERT(t, chunks[i-1]->LoadAddress() < chunks[i]->LoadAddress());
    }

    auto ch100 = segment->ChunkFromAddress(104);
    TR_ASSERT(t, ch100->LoadAddress() == 100);

    return kTR_Pass;
}
DLL_EXPORT int test_segment_chunkfromaddr(ITesting *t) {
    auto segment = std::make_shared<Segment>("name", 0);
    int byte = 0x00;
    for(int i=0;i<16;i++, byte++) segment->WriteByte(byte);
    segment->CreateChunk(100);
    for(int i=0;i<16;i++, byte++) segment->WriteByte(byte);
    segment->CreateChunk(200);
    for(int i=0;i<16;i++, byte++) segment->WriteByte(byte);
    segment->CreateChunk(300);
    for(int i=0;i<16;i++, byte++) segment->WriteByte(byte);

    Segment::DataChunk::Ref chunk;
    chunk = segment->ChunkFromAddress(101);
    TR_ASSERT(t, chunk != nullptr);
    TR_ASSERT(t, chunk->LoadAddress() == 100);
    chunk = segment->ChunkFromAddress(100);
    TR_ASSERT(t, chunk->LoadAddress() == 100);

    return kTR_Pass;

}
