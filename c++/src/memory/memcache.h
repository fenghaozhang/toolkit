#ifndef _SRC_MEMORY_MEMCACHE_H
#define _SRC_MEMORY_MEMCACHE_H

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#include <list>
#include <string>

#include "src/base/intrusive_list.h"
#include "src/common/macros.h"

struct MemCacheOptions
{
    std::string name;
    void (*ctor)(void* obj);
    void (*dtor)(void* obj);
    uint32_t objSize;
    uint32_t limit;
    uint32_t reserve;

    MemCacheOptions()
        : name("unamed_obj_cache"),
          ctor(NULL),
          dtor(NULL),
          objSize(0),
          limit(INT_MAX),
          reserve(16)
    {
    }
};

struct MemCacheStat
{
    std::string name;
    uint32_t pages;
    uint32_t pageSize;
    uint32_t itemSize;
    uint32_t objSize;
    uint32_t objCount;
    uint32_t objPerPage;
    uint32_t reservedPages;
    uint32_t maxReservedPages;

    MemCacheStat()
        : pages(0), pageSize(0), itemSize(0),
          objSize(0), objCount(0), objPerPage(0),
          reservedPages(0), maxReservedPages(0)
    {
    }
};

class MemCache
{
public:
    typedef MemCacheOptions Options;

    MemCache();
    virtual ~MemCache();

    /** NOTE: Must call Init() before allocation */
    void Init(const Options& options);

    /** Get stat of memcache */
    void GetStats(MemCacheStat* options) const;

    void* Alloc();
    void  Dealloc(void* ptr);

private:
    friend class MemCacheTest_FreeAllPages_Test;
    struct PageHeader;

    void freeCache();
    void createPage();
    void freePage(PageHeader* page);
    void freeAllPages();
    void adjustPageAtAlloc(PageHeader* page);
    void adjustPageAtDealloc(PageHeader* page);
    PageHeader* initPage(void* page);
    PageHeader* findPage(void* ptr);
    PageHeader* findOrCreatePage();
    void freeObjects(PageHeader* page);
    void* allocObj(PageHeader* page) const;
    void  deallocObj(PageHeader* page, void* ptr) const;
    void addToGlobalList();
    void removeFromGlobalList();

private:
    struct ItemHeader
    {
        ItemHeader* next;
    };

    struct PageHeader
    {
        ItemHeader* freeList;
        uint32_t usedCount;
        uint32_t freeCount;
        LinkNode node;
    } __attribute__((aligned(64)));
    typedef IntrusiveList<PageHeader, &PageHeader::node> PageList;
    PageList mEmptyPages;
    PageList mPartialPages;
    PageList mFullPages;

    Options  mOptions;
    bool     mIsInited;
    uint32_t mPages;
    uint32_t mPageSize;
    uint32_t mItems;
    uint32_t mItemSize;
    uint32_t mItemsPerPage;
    uint32_t mReservedPages;
    uint32_t mMaxReservedPages;

    LinkNode mNode;
    typedef IntrusiveList<MemCache, &MemCache::mNode> CacheList;
    static CacheList sInstanceList;
    static pthread_mutex_t* sInstanceLock;

    DISALLOW_COPY_AND_ASSIGN(MemCache);
};

#endif  // _SRC_MEMORY_MEMCACHE_H
