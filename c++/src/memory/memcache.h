#ifndef _SRC_MEMORY_MEMPOOL_H
#define _SRC_MEMORY_MEMPOOL_H

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include <list>

#include "src/base/intrusive_list.h"
#include "src/common/macro.h"

#define MIN_PAGE_SIZE (64 * 1024)

struct ItemHeader
{
    ItemHeader* next;

    ItemHeader(): next(NULL) {}
};

struct PageHeader
{
    ItemHeader* freeList;
    uint32_t freeCount;
    LinkNode node;

    PageHeader() : freeList(NULL), freeCount(0) {}

    void* Alloc();
    void Dealloc(void* ptr);
} __attribute__((aligned(64)));

class MemCache
{
public:
    explicit MemCache(size_t objSize);
    ~MemCache();

    void* Alloc();
    void  Dealloc(void* ptr);

private:
    PageHeader* initPage(void* page);
    void createFreePage();
    void adjustPageAtAlloc(PageHeader* page);
    void adjustPageAtDealloc(PageHeader* page);
    PageHeader* findPage(void* ptr);
    PageHeader* findOrCreatePage();

    typedef IntrusiveList<PageHeader, &PageHeader::node> PageList;
    PageList mFreePages;
    PageList mPartialPages;
    PageList mFullPages;

    const size_t mMaxItems;
    const size_t mItemSize;
    const size_t mPageSize;

    DISALLOW_COPY_AND_ASSIGN(MemCache);
};

template <typename obj>
class ObjPool : public MemCache
{
    // TODO(allen.zfh): do something
};

#endif  // _SRC_MEMORY_MEMPOOL_H
