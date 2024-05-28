//
// Created by gnilk on 28.05.24.
//
#include <stdint.h>
#include <testinterface.h>

#include "Ringbuffer.h"

using namespace gnilk;
using namespace gnilk::vcpu;


extern "C" {
DLL_EXPORT int test_ringbuffer(ITesting *t);
DLL_EXPORT int test_ringbuffer_write(ITesting *t);
DLL_EXPORT int test_ringbuffer_read(ITesting *t);
DLL_EXPORT int test_ringbuffer_write_read_wrap(ITesting *t);
DLL_EXPORT int test_ringbuffer_write_read_wrap2(ITesting *t);
}
DLL_EXPORT int test_ringbuffer(ITesting *t) {
    return kTR_Pass;
}
DLL_EXPORT int test_ringbuffer_write(ITesting *t) {
    Ringbuffer<32> ringbuffer;
    TR_ASSERT(t, ringbuffer.IsFull() == false);

    for(int i=0;i<32;i++) {
        uint8_t data = i;
        TR_ASSERT(t, ringbuffer.Write(&data, sizeof(data) == sizeof(data)));
    }
    TR_ASSERT(t, ringbuffer.IsFull());
    TR_ASSERT(t, ringbuffer.BytesAvailable() == 32);
    TR_ASSERT(t, ringbuffer.BytesFree() == 0);

    uint8_t overflow = 1;
    TR_ASSERT(t, ringbuffer.Write(&overflow, sizeof(overflow)) < 0);
    return kTR_Pass;
}

DLL_EXPORT int test_ringbuffer_read(ITesting *t) {
    Ringbuffer<32> ringbuffer;
    TR_ASSERT(t, ringbuffer.IsFull() == false);

    for(int i=0;i<32;i++) {
        uint8_t data = i;
        TR_ASSERT(t, ringbuffer.Write(&data, sizeof(data) == sizeof(data)));
    }
    TR_ASSERT(t, ringbuffer.IsFull());
    TR_ASSERT(t, ringbuffer.BytesAvailable() == 32);
    TR_ASSERT(t, ringbuffer.BytesFree() == 0);
    for (int i=0;i<32;i++) {
        uint8_t out;
        TR_ASSERT(t, ringbuffer.Read(&out, sizeof(out)) == 1);
        TR_ASSERT(t, !ringbuffer.IsFull());
    }
    TR_ASSERT(t, ringbuffer.BytesAvailable() == 0);
    TR_ASSERT(t, ringbuffer.BytesFree() == 32);

    uint8_t underflow = 1;
    TR_ASSERT(t, ringbuffer.Read(&underflow, sizeof(underflow)) < 0);
    return kTR_Pass;

}


DLL_EXPORT int test_ringbuffer_write_read_wrap(ITesting *t) {
    Ringbuffer<32> ringbuffer;
    TR_ASSERT(t, ringbuffer.IsFull() == false);

    uint8_t dummy[24];

    // Move 'idxWrite' up to 24
    TR_ASSERT(t, ringbuffer.Write(dummy, sizeof(dummy)) == sizeof(dummy));
    TR_ASSERT(t, ringbuffer.BytesAvailable() == 24);

    // Move 'idxRead' up to 24
    TR_ASSERT(t, ringbuffer.Read(dummy, sizeof(dummy)) == sizeof(dummy));

    // Now - write another 24 bytes, causing idxWrite to pass beyond 32
    TR_ASSERT(t, ringbuffer.Write(dummy, sizeof(dummy)) > 0);

    TR_ASSERT(t, ringbuffer.Read(dummy, sizeof(dummy)) > 0);

    return kTR_Pass;
}

DLL_EXPORT int test_ringbuffer_write_read_wrap2(ITesting *t) {
    Ringbuffer<32> ringbuffer;
    TR_ASSERT(t, ringbuffer.IsFull() == false);

    uint8_t dummy[32];

    // Move 'idxWrite' up to 32 - making it wrap to 0
    TR_ASSERT(t, ringbuffer.Write(dummy, 32) == 32);
    TR_ASSERT(t, ringbuffer.BytesAvailable() == 32);

    // now move 'idxRead' up to 24 so that 'idxWrite < idxRead'
    TR_ASSERT(t, ringbuffer.Read(dummy, 24) == 24);

    // Now write - this will stress the first condition in the write logic
    TR_ASSERT(t, ringbuffer.Write(dummy, 1) == 1);
    TR_ASSERT(t, ringbuffer.Write(dummy, 1) == 1);

    return kTR_Pass;
}

