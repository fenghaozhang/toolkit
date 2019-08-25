#include "src/memory/memcache.h"

#include <malloc.h>
#include <unordered_set>

#include "src/common/assert.h"
#include "src/common/macros.h"
#include "src/math/math.h"

static const size_t MIN_PAGE_SIZE = 64 * 1024;
static const size_t MIN_OBJS_PER_PAGE = 8;

static pthread_mutex_t* initLock();
MemCache::CacheList MemCache::sInstanceList;
pthread_mutex_t* MemCache::sInstanceLock = initLock();

static pthread_mutex_t* initLock()
{
    static pthread_mutex_t sInstanceLock_;
    pthread_mutex_init(&sInstanceLock_, NULL);
    return &sInstanceLock_;
}

MemCache::MemCache()
    : mIsInited(false),
      mPages(0),
      mPageSize(0),
      mItems(0),
      mItemSize(0),
      mItemsPerPage(0),
      mReservedPages(0),
      mMaxReservedPages(0)
{
}

MemCache::~MemCache()
{
    freeCache();
}

void MemCache::Init(const Options& options)
{
    mOptions = options;
    if (UNLIKELY(mOptions.objSize == 0))
    {
        // TODO(allen.zfh): Add error log
        return;
    }
    if (UNLIKELY(mOptions.objSize < sizeof(ItemHeader)))
    {
        // Object's head erea is also used to store header
        // So there is waste when object is smaller than header
        // TODO(allen.zfh): Add warning log
    }
    mItemSize = MAX(mOptions.objSize, sizeof(ItemHeader));
    mPageSize = MIN_PAGE_SIZE;
    while (mPageSize < (mItemSize * MIN_OBJS_PER_PAGE + sizeof(PageHeader)))
    {
        mPageSize *= 2;
    }
    ASSERT(IsPowerOfTwo(mPageSize));
    mItemsPerPage = (mPageSize - sizeof(PageHeader)) / mItemSize;
    uint32_t reservedObjs = MIN(mOptions.reserve, mOptions.limit);
    mMaxReservedPages = (reservedObjs + mItemsPerPage - 1) / mItemsPerPage;
    for (uint32_t i = 0; i < mMaxReservedPages; ++i)
    {
        createPage();
    }
    ASSERT(mMaxReservedPages == mReservedPages &&
           mMaxReservedPages == mPages);
    addToGlobalList();
    mIsInited = true;
}

void* MemCache::Alloc()
{
    if (UNLIKELY(!mIsInited))
    {
        // TODO(allen.zfh): Add warning log
        return NULL;
    }
    if (mItems >= mOptions.limit)
    {
        // TODO(allen.zfh): Add warning log
        return NULL;
    }
    ++mItems;
    PageHeader* page = findOrCreatePage();
    void* obj = allocObj(page);
    adjustPageAtAlloc(page);
    return obj;
}

void MemCache::Dealloc(void* ptr)
{
    if (UNLIKELY(ptr == NULL))
    {
        return;
    }
    --mItems;
    PageHeader* page = findPage(ptr);
    deallocObj(page, ptr);
    adjustPageAtDealloc(page);
}

void MemCache::GetStats(MemCacheStat* stat) const
{
    stat->name = mOptions.name;
    stat->pages = mPages;
    stat->pageSize = mPageSize;
    stat->itemSize = mItemSize;
    stat->objSize = mOptions.objSize;
    stat->objCount = mItems;
    stat->objPerPage = mItemsPerPage;
    stat->reservedPages = mReservedPages;
    stat->maxReservedPages = mMaxReservedPages;
}

void MemCache::freeCache()
{
    if (UNLIKELY(mIsInited))
    {
        freeAllPages();
        removeFromGlobalList();
    }
}

void MemCache::createPage()
{
    void* page = memalign(mPageSize, mPageSize);
    PageHeader* header = initPage(page);
    mEmptyPages.push_back(header);
    ++mReservedPages;
    ++mPages;
}

void MemCache::freeObjects(PageHeader* page)
{
    if (mOptions.dtor == NULL)
    {
        mItems = 0;
        return;
    }
    std::unordered_set<ItemHeader*> freeItems;
    ItemHeader* item = page->freeList;
    while (item != NULL)
    {
        freeItems.insert(item);
        item = item->next;
    }
    char* pos = reinterpret_cast<char*>(page + 1);
    for (uint32_t i = 0; i < mItemsPerPage; ++i)
    {
        item = reinterpret_cast<ItemHeader*>(pos);
        pos += mItemSize;
        if (freeItems.find(item) == freeItems.end())
        {
            mOptions.dtor(item);
            --mItems;
        }
    }
}

void MemCache::freePage(PageHeader* page)
{
    if (UNLIKELY(page->usedCount != 0))
    {
        // It's possible user forgets to free memory explicitly
        // TODO(allen.zfh) : warning
        freeObjects(page);
    }
    page->node.Unlink();
    free(page);
    --mPages;
}

void MemCache::freeAllPages()
{
    PageList pages;
    pages.splice(pages.end(), mEmptyPages.begin(), mEmptyPages.end());
    pages.splice(pages.end(), mPartialPages.begin(), mPartialPages.end());
    pages.splice(pages.end(), mFullPages.begin(), mFullPages.end());
    ASSERT_DEBUG(mEmptyPages.empty());
    ASSERT_DEBUG(mPartialPages.empty());
    ASSERT_DEBUG(mFullPages.empty());
    FOREACH_SAFE(iter, pages)
    {
        freePage(&*iter);
    }
    ASSERT_DEBUG(mItems == 0);
    ASSERT_DEBUG(mPages == 0);
    ASSERT_DEBUG(mReservedPages <= mMaxReservedPages);
}

void MemCache::adjustPageAtAlloc(PageHeader* page)
{
    ASSERT_DEBUG(page->freeCount >= 0 && page->freeCount <= mItemsPerPage);
    ASSERT_DEBUG(page->freeCount + page->usedCount == mItemsPerPage);
    if (UNLIKELY(page->freeCount == 0))
    {
        ASSERT_DEBUG(page->freeList == NULL);
        mPartialPages.erase(page);
        mFullPages.push_back(page);
    }
    else if (UNLIKELY(page->freeCount == mItemsPerPage - 1))
    {
        --mReservedPages;
        mEmptyPages.erase(page);
        mPartialPages.push_back(page);
    }
}

void MemCache::adjustPageAtDealloc(PageHeader* page)
{
    ASSERT_DEBUG(page->freeCount >= 0 && page->freeCount <= mItemsPerPage);
    ASSERT_DEBUG(page->freeCount + page->usedCount == mItemsPerPage);
    if (UNLIKELY(page->freeCount == 1))
    {
        mFullPages.erase(page);
        mPartialPages.push_back(page);
    }
    else if (UNLIKELY(page->freeCount == mItemsPerPage))
    {
        mPartialPages.erase(page);
        if (mReservedPages >= mMaxReservedPages)
        {
            freePage(page);
        }
        else
        {
            mEmptyPages.push_back(page);
            ++mReservedPages;
        }
    }
}

MemCache::PageHeader* MemCache::initPage(void* page)
{
    PageHeader* header = static_cast<PageHeader*>(page);
    char* pos = static_cast<char*>(page) + sizeof(PageHeader);
    header->freeList = reinterpret_cast<ItemHeader*>(pos);
    header->freeCount = mItemsPerPage;
    header->usedCount = 0;

    ItemHeader* lastHeader = NULL;
    for (uint32_t i = 0; i < mItemsPerPage; ++i)
    {
        lastHeader = reinterpret_cast<ItemHeader*>(pos);
        pos += mItemSize;
        lastHeader->next = reinterpret_cast<ItemHeader*>(pos);
    }
    lastHeader->next = NULL;
    return header;
}

MemCache::PageHeader* MemCache::findPage(void* page)
{
    uint64_t addr = reinterpret_cast<uint64_t>(page);
    addr &= ~(mPageSize - 1);
    return reinterpret_cast<PageHeader*>(addr);
}

MemCache::PageHeader* MemCache::findOrCreatePage()
{
    if (!mPartialPages.empty())
    {
        return mPartialPages.front();
    }
    if (mEmptyPages.empty())
    {
        createPage();
    }
    return mEmptyPages.front();
}

void* MemCache::allocObj(PageHeader* page) const
{
    ASSERT_DEBUG(page->freeList != NULL && page->freeCount != 0);
    ItemHeader* item = page->freeList;
    page->freeList = item->next;
    --page->freeCount;
    ++page->usedCount;
    item->next = NULL;
    ASSERT_DEBUG(page->usedCount <= mItemsPerPage);
    if (mOptions.ctor != NULL)
    {
        mOptions.ctor(item);
    }
    return item;
}

void MemCache::deallocObj(PageHeader* page, void* ptr) const
{
    if (mOptions.dtor != NULL)
    {
        mOptions.dtor(ptr);
    }
    ItemHeader* item = static_cast<ItemHeader*>(ptr);
    item->next = page->freeList;
    page->freeList = item;
    ++page->freeCount;
    --page->usedCount;
    ASSERT_DEBUG(page->freeCount <= mItemsPerPage);
}

void MemCache::addToGlobalList()
{
    pthread_mutex_lock(sInstanceLock);
    sInstanceList.push_back(this);
    pthread_mutex_unlock(sInstanceLock);
}

void MemCache::removeFromGlobalList()
{
    pthread_mutex_lock(sInstanceLock);
    sInstanceList.erase(this);
    pthread_mutex_unlock(sInstanceLock);
}
