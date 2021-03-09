#ifndef __LIST_H__
#define __LIST_H__

#include "Types.h"

// Structure
#pragma pack(push, 1)

// Must be in front of the data
typedef struct kListLinkStruct {
    void *pNext;    // Address of Next Data
    QWORD id;       // Unique ID
} LISTLINK;

/*
struct kListItemExampleStruct {
    LISTLINK listLink;

    int i;
    char c;
};
*/

// Data Structure to manage list
typedef struct kListManagerStruct {
    int itemCount;

    void *pHead;    // Address of First Item
    void *pTail;    // Address of Last Item
} LIST;

#pragma pack(pop)

// Function
void kInitializeList(LIST *pList);
int kGetListCount(const LIST *pList);
void kAddListToTail(LIST *pList, void *pItems);
void kAddListToHead(LIST *pList, void *pItems);
void *kRemoveList(LIST *pList, QWORD id);
void *kRemoveListFromHead(LIST *pList);
void *kRemoveListFromTail(LIST *pList);
void *kFindList(const LIST *pList, QWORD id);
void *kGetHeadFromList(const LIST *pList);
void *kGetTailFromList(const LIST *pList);
void *kGetNextFromList(const LIST *pList, void *pCurItem);

#endif  // __LIST_H__