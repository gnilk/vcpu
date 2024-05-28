//
// Created by gnilk on 28.05.24.
//
#include <stdint.h>
#include "Dispatch.h"
#include <testinterface.h>

using namespace gnilk;
using namespace gnilk::vcpu;


extern "C" {
DLL_EXPORT int test_dispatch(ITesting *t);
DLL_EXPORT int test_dispatch_push_pop_single(ITesting *t);
DLL_EXPORT int test_dispatch_push_pop_many(ITesting *t);
}
DLL_EXPORT int test_dispatch(ITesting *t) {
    return kTR_Pass;
}


DLL_EXPORT int test_dispatch_push_pop_single(ITesting *t) {
    Dispatch<64> dispatch;
    TR_ASSERT(t, dispatch.IsEmpty());

    struct Item {
        int value;
    };
    Item inItem = {.value = 4711 };
    Item outItem;
    TR_ASSERT(t, dispatch.Push(0,&inItem, sizeof(inItem)));
    TR_ASSERT(t, !dispatch.IsEmpty());
    TR_ASSERT(t, dispatch.Pop(&outItem, sizeof(outItem)));
    TR_ASSERT(t, outItem.value == 4711);
    TR_ASSERT(t, dispatch.IsEmpty());

    return kTR_Pass;
}
DLL_EXPORT int test_dispatch_push_pop_many(ITesting *t) {
    Dispatch<12> dispatch;
    TR_ASSERT(t, dispatch.IsEmpty());

    struct Item {
        int32_t value;
    };

    Item inItem = {.value = 4711 };
    while(dispatch.CanInsert(sizeof(Item))) {
        TR_ASSERT(t, dispatch.Push(0,&inItem, sizeof(inItem)));
        inItem.value++;
    }
    TR_ASSERT(t, !dispatch.IsEmpty());

    int32_t expected = 4711;
    Item outItem;
    while(!dispatch.IsEmpty()) {
        TR_ASSERT(t, dispatch.Pop(&outItem, sizeof(outItem)));
        TR_ASSERT(t, outItem.value == expected);
        expected++;
    }

    TR_ASSERT(t, dispatch.IsEmpty());

    return kTR_Pass;
}