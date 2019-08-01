#include "src/memory/memcache.h"

#include <malloc.h>

#include "src/common/assert.h"
#include "src/common/macro.h"

void* PageHeader::Alloc()
{
    ASSERT(freeList != NULL && freeCount != 0);
    ItemHeader* item = freeList;
    freeList = freeList->next;
    item->next = NULL;
    --freeCount;
    return item;
}

void PageHeader::Dealloc(void* ptr)
{
    ItemHeader* item = reinterpret_cast<ItemHeader*>(ptr);
    item->next = freeList;
    freeList = item;
    ++freeCount;
}

MemCache::MemCache(size_t objSize)
    : mMaxItems((MIN_PAGE_SIZE - sizeof(PageHeader)) / objSize),
      mItemSize(objSize),
      mPageSize(MIN_PAGE_SIZE)
{
}

MemCache::~MemCache()
{
}

PageHeader* MemCache::initPage(void* page)
{
    PageHeader* header = reinterpret_cast<PageHeader*>(page);
    char* pos = reinterpret_cast<char*>(page) + sizeof(PageHeader);
    header->freeList = reinterpret_cast<ItemHeader*>(pos);
    header->freeCount = mMaxItems;

    ItemHeader* lastHeader = NULL;
    for (uint32_t i = 0; i < mMaxItems; ++i)
    {
        lastHeader = reinterpret_cast<ItemHeader*>(pos);
        pos += mItemSize;
        lastHeader->next = reinterpret_cast<ItemHeader*>(pos);
    }
    lastHeader->next = NULL;
    return header;
}

void MemCache::createFreePage()
{
    void* page = memalign(mPageSize, mPageSize);
    PageHeader* header = initPage(page);
    mFreePages.push_back(header);
}

PageHeader* MemCache::findOrCreatePage()
{
    if (!mPartialPages.empty())
    {
        return mPartialPages.front();
    }
    if (mFreePages.empty())
    {
        createFreePage();
    }
    return mFreePages.front();
}

void MemCache::adjustPageAtAlloc(PageHeader* page)
{
    ASSERT(page->freeCount >= 0 && page->freeCount <= mMaxItems);
    if (UNLIKELY(page->freeCount == 0))
    {
        mPartialPages.erase(page);
        mFullPages.push_back(page);
    }
    else if (UNLIKELY(page->freeCount == mMaxItems - 1))
    {
        mFreePages.erase(page);
        mPartialPages.push_back(page);
    }
}

void MemCache::adjustPageAtDealloc(PageHeader* page)
{
    ASSERT(page->freeCount >= 0 && page->freeCount <= mMaxItems);
    if (UNLIKELY(page->freeCount == 1))
    {
        mFullPages.erase(page);
        mPartialPages.push_back(page);
    }
    else if (UNLIKELY(page->freeCount == mMaxItems))
    {
        mPartialPages.erase(page);
        mFreePages.push_back(page);
    }
}

PageHeader* MemCache::findPage(void* ptr)
{
    uint64_t addr = reinterpret_cast<uint64_t>(ptr);
    addr &= ~(mPageSize - 1);
    return reinterpret_cast<PageHeader*>(addr);
}

void* MemCache::Alloc()
{
    PageHeader* page = findOrCreatePage();
    void* item = page->Alloc();
    adjustPageAtAlloc(page);
    return item;
}

void MemCache::Dealloc(void* ptr)
{
    PageHeader* page = findPage(ptr);
    page->Dealloc(ptr);
    ASSERT(page->freeCount <= mMaxItems);
    adjustPageAtDealloc(page);
}
