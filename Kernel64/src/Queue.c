#include "Queue.h"
#include "Utility.h"

// Initialize Queue
void kInitializeQueue(QUEUE *pQueue, void *pQueueBuf, int maxDataCnt, int dataSize) {
    pQueue->maxDataCnt = maxDataCnt;
    pQueue->dataSize = dataSize;
    pQueue->pQueueArray = pQueueBuf;
}

BOOL kIsQueueFull(const QUEUE *pQueue) {
    return pQueue->getIdx == pQueue->putIdx && pQueue->isLastOpPut;
}

BOOL kIsQueueEmpty(const QUEUE *pQueue) {
    return pQueue->getIdx == pQueue->putIdx && !(pQueue->isLastOpPut);
}

BOOL kPutQueue(QUEUE *pQueue, const void *pData) {
    if(kIsQueueFull(pQueue))
        return FALSE;

    // putIdx 위치의 데이터를, 데이터 크기만큼 복사
    kMemCpy((char *)pQueue->pQueueArray + (pQueue->dataSize * pQueue->putIdx), pData, pQueue->dataSize);

    // Modify putIdx
    pQueue->putIdx = (pQueue->putIdx + 1) % pQueue->maxDataCnt;
    pQueue->isLastOpPut = TRUE;

    return TRUE;
}

BOOL kGetQueue(QUEUE *pQueue, void *pData) {
    if(kIsQueueEmpty(pQueue))
        return FALSE;

    // putIdx 위치의 데이터를, 데이터 크기만큼 복사
    kMemCpy((char *)pQueue->pQueueArray + (pQueue->dataSize * pQueue->getIdx), pData, pQueue->dataSize);

    // Modify getIdx
    pQueue->getIdx = (pQueue->getIdx + 1) % pQueue->maxDataCnt;
    pQueue->isLastOpPut = FALSE;

    return TRUE;
}