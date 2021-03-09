#include "List.h"

// Initialize List
void kInitializeList(LIST *pList) {
    pList->itemCount = 0;
    pList->pHead = NULL;
    pList->pTail = NULL;
}

// Return Count of List
int kGetListCount(const LIST *pList) {
    return pList->itemCount;
}

// Add Items after tail of list
void kAddListToTail(LIST *pList, void *pItems) {
    LISTLINK *pLink;

    // Set Added datas' address to NULL
    pLink = (LISTLINK *)pItems;
    pLink->pNext = NULL;

    if(pList->pHead == NULL) {
        pList->pHead = pItems;
        pList->pTail = pItems;
        pList->itemCount = 1;

        return;
    }

    pLink = (LISTLINK *)pList->pTail;
    pLink->pNext = pItems;

    pList->pTail = pItems;
    pList->itemCount++;
}

void kAddListToHead(LIST *pList, void *pItems) {
    LISTLINK *pLink;

    pLink = (LISTLINK *)pItems;
    pLink->pNext = pList->pHead;

    if(pList->pHead == NULL) {
        pList->pHead = pItems;
        pList->pTail = pItems;
        pList->itemCount = 1;

        return;
    }

    pList->pHead = pItems;
    pList->itemCount++;
}

void *kRemoveList(LIST *pList, QWORD id) {
    LISTLINK *pLink, *pPrevLink;

    pPrevLink = (LISTLINK *)pList->pHead;
    for(pLink = pPrevLink; pLink != NULL; pLink = pLink->pNext) {
        if(pLink->id == id) {
            if(pLink == pList->pHead && pLink == pList->pTail) {
                pList->pHead = NULL;
                pList->pTail = NULL;
            }
            else if(pLink == pList->pHead)
                pList->pHead = pLink->pNext;
            else if(pLink == pList->pTail)
                pList->pTail = pPrevLink;
            else
                pPrevLink->pNext = pLink->pNext;

            pList->itemCount--;
            return pLink;
        }
        pPrevLink = pLink;
    }

    return NULL;
}

void *kRemoveListFromHead(LIST *pList) {
    LISTLINK *pLink;

    if(pList->itemCount == 0)
        return NULL;

    // Remove Head & Return
    pLink = (LISTLINK *)pList->pHead;
    
    return kRemoveList(pList, pLink->id);
}

void *kRemoveListFromTail(LIST *pList) {
    LISTLINK *pLink;

    if(pList->itemCount == 0)
        return NULL;

    // Remove Head & Return
    pLink = (LISTLINK *)pList->pTail;
    
    return kRemoveList(pList, pLink->id);
}

void *kFindList(const LIST *pList, QWORD id) {
    LISTLINK *pLink;

    for(pLink = (LISTLINK *)pList->pHead; pLink != NULL; pLink = pLink->pNext) {
        if(pLink->id == id)
            return pLink;
    }

    return NULL;
}

void *kGetHeadFromList(const LIST *pList) {
    return pList->pHead;
}

void *kGetTailFromList(const LIST *pList) {
    return pList->pTail;
}

void *kGetNextFromList(const LIST *pList, void *pCurItem) {
    // pList is required later
    return pCurItem ? ((LISTLINK *)pCurItem)->pNext : NULL;
}