#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "Types.h"

#pragma pack(push, 1)

typedef struct kQueueManagerStruct {
    int dataSize;
    int maxDataCnt;

    void *pQueueArray;
    int putIdx;
    int getIdx;

    BOOL isLastOpPut;
} QUEUE;

#pragma pack(pop)

void kInitializeQueue(QUEUE *pQueue, void *pQueueBuf, int maxDataCnt, int dataSize);
BOOL kIsQueueFull(const QUEUE *pQueue);
BOOL kIsQueueEmpty(const QUEUE *pQueue);
BOOL kPutQueue(QUEUE *pQueue, const void *pData);
BOOL kGetQueue(QUEUE *pQueue, void *pData);

#endif  // __QUEUE_H__