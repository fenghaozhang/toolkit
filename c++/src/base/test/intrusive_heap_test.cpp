#include <queue>
#include <vector>
#include <gtest/gtest.h>

#include "src/base/intrusive_heap.h"
#include "src/base/intrusive_list.h"
#include "src/memory/mempool.h"

class IntrusiveHeapTest : public testing::Test
{
public:
    IntrusiveHeapTest() {}

    void SetUp() { srand(unsigned(time(NULL))); }

    void TearDown() {}
};

struct Record
{
    int value;

    HeapNode heapNode;
    LinkNode listNode;

    Record(): value(0) { }
    explicit Record(int v): value(v) { }

    bool operator==(const Record& rhs) const { return value == rhs.value; }
    bool operator<(const Record& rhs) const { return value < rhs.value; }
    bool operator>(const Record& rhs) const { return value > rhs.value; }
};

struct RecordCompare
{
    bool operator()(const Record* left, const Record* right)
    {
        return left->value < right->value;
    }
};

typedef IntrusiveHeap<
    Record, &Record::heapNode, std::less<Record> > MaxHeap;
typedef IntrusiveHeap<
    Record, &Record::heapNode, std::greater<Record> > MinHeap;

TEST_F(IntrusiveHeapTest, Push)
{
    MemPool pool;
    MaxHeap heap;
    heap.reserve(1024);

    // 1. Insert objects
    for (size_t i = 0; i < 10240; ++i)
    {
        int value = rand();    // NOLINT(runtime/threadsafe_fn)
        Record* record = pool.New<Record>(value);
        heap.push(record);
    }
}

TEST_F(IntrusiveHeapTest, Pop1)
{
    MemPool pool;
    MaxHeap heap;
    heap.reserve(1024);

    // 1. Insert objects
    for (size_t i = 0; i < 2048; ++i)
    {
        int value = rand();    // NOLINT(runtime/threadsafe_fn)
        Record* record = pool.New<Record>(value);
        heap.push(record);
    }

    // 2. Pop objects and check order
    int prevValue = INT32_MAX;
    while (!heap.empty())
    {
        Record* record = heap.top();
        EXPECT_GE(prevValue, record->value);
        prevValue = record->value;
        heap.pop();
        EXPECT_TRUE(record->heapNode.IsSingle());
    }
    EXPECT_TRUE(heap.empty());
}

TEST_F(IntrusiveHeapTest, Pop2)
{
    MemPool pool;
    MinHeap minHeap;
    minHeap.reserve(1024);

    // 1. Insert objects
    for (size_t i = 0; i < 1024; ++i)
    {
        int value = rand();    // NOLINT(runtime/threadsafe_fn)
        Record* record = pool.New<Record>(value);
        minHeap.push(record);
    }

    // 2. Pop objects and check order
    int prevValue = INT32_MIN;
    while (!minHeap.empty())
    {
        Record* record = minHeap.top();
        EXPECT_LE(prevValue, record->value);
        prevValue = record->value;
        minHeap.pop();
        EXPECT_TRUE(record->heapNode.IsSingle());
    }
    EXPECT_TRUE(minHeap.empty());
}

TEST_F(IntrusiveHeapTest, Clear)
{
    MemPool pool;
    MaxHeap heap;
    heap.reserve(1024);

    // 1. Clear all objects
    for (size_t i = 0; i < 2048; ++i)
    {
        int value = rand();    // NOLINT(runtime/threadsafe_fn)
        Record* record = pool.New<Record>(value);
        heap.push(record);
    }
    heap.clear();
    EXPECT_TRUE(heap.empty());
}

TEST_F(IntrusiveHeapTest, Erase)
{
    MemPool pool;
    MaxHeap heap;
    heap.reserve(1024);

    // 1. Insert objects
    std::vector<Record*> toBeDeleted;
    Record* lastRecord = NULL;
    for (size_t i = 0; i < 2048; ++i)
    {
        int value = rand();    // NOLINT(runtime/threadsafe_fn)
        Record* record = pool.New<Record>(value);
        heap.push(record);
        lastRecord = record;
        if (rand() % 3 == 0)
        {
            toBeDeleted.push_back(record);
        }
    }

    // 2. Erase last and other random objects
    heap.erase(lastRecord);
    EXPECT_TRUE(lastRecord->heapNode.IsSingle());
    size_t count = heap.size();
    if (lastRecord == toBeDeleted[toBeDeleted.size() - 1])
    {
        toBeDeleted.pop_back();
    }
    FOREACH(iter, toBeDeleted)
    {
        heap.erase(*iter);
        --count;
        EXPECT_TRUE((*iter)->heapNode.IsSingle());
    }
    EXPECT_EQ(count, heap.size());

    // 3. Re-check order after erase
    int prevValue = INT32_MAX;
    while (!heap.empty())
    {
        Record* record = heap.top();
        EXPECT_GE(prevValue, record->value);
        prevValue = record->value;
        heap.pop();
        EXPECT_TRUE(record->heapNode.IsSingle());
    }
    EXPECT_TRUE(heap.empty());
}

TEST_F(IntrusiveHeapTest, CompareWithPriorityQueue)
{
    MemPool pool;
    const size_t HEAP_SIZE = 10000;
    MaxHeap heap;
    heap.reserve(HEAP_SIZE);
    std::priority_queue<Record*, std::vector<Record*>, RecordCompare> queue;

    // 1. Insert objects into IntrusiveHeap and std::priority_queue
    for (size_t round = 0; round < HEAP_SIZE; ++round)
    {
        if (rand() % 2 == 0)    // NOLINT(runtime/threadsafe_fn)
        {
            int value = rand();    // NOLINT(runtime/threadsafe_fn)
            Record* record = pool.New<Record>(value);
            heap.push(record);
            queue.push(record);
        }
        if (rand() % 2 == 0)    // NOLINT(runtime/threadsafe_fn)
        {
            if (!heap.empty())
            {
                Record* record = heap.top();
                EXPECT_EQ(record, queue.top());
                heap.pop();
                queue.pop();
                EXPECT_TRUE(record->heapNode.IsSingle());
            }
        }
    }

    // 2. Compare results between IntrusiveHeap and std::priority_queue
    EXPECT_EQ(heap.size(), queue.size());
    while (!heap.empty())
    {
        const Record* record1 = heap.top();
        const Record* record2 = queue.top();
        EXPECT_EQ(*record1, *record2);
        EXPECT_EQ(record1, record2);
        heap.pop();
        queue.pop();
        EXPECT_TRUE(record1->heapNode.IsSingle());
    }
}

TEST_F(IntrusiveHeapTest, ConstructWithIntrusiveList)
{
    MemPool pool;
    IntrusiveList<Record, &Record::listNode> list;
    std::priority_queue<Record*, std::vector<Record*>, RecordCompare> queue;
    size_t capacity = 10240;
    for (size_t i = 0; i < capacity; ++i)
    {
        int value = rand();    // NOLINT(runtime/threadsafe_fn)
        Record* record = pool.New<Record>(value);
        list.push_back(record);
        queue.push(record);
    }

    MaxHeap heap(list.begin(), list.end());
    EXPECT_EQ(capacity, heap.size());
    EXPECT_EQ(capacity, queue.size());
    while (!heap.empty())
    {
        const Record* record1 = heap.top();
        const Record* record2 = queue.top();
        EXPECT_EQ(*record1, *record2);
        heap.pop();
        queue.pop();
        EXPECT_TRUE(record1->heapNode.IsSingle());
    }
    EXPECT_TRUE(heap.empty());
}

TEST_F(IntrusiveHeapTest, Reserve)
{
    MemPool pool;
    MaxHeap heap;

    // 1. Insert objects
    size_t i = 0;
    for (; i < 10240; ++i)
    {
        int value = rand();    // NOLINT(runtime/threadsafe_fn)
        Record* record = pool.New<Record>(value);
        heap.push(record);
        EXPECT_EQ(heap.size(), i + 1);
        EXPECT_GE(heap.capacity(), i + 1);
    }

    // 2. Mannual reserve
    size_t newCapacity = heap.capacity() * 2;
    heap.reserve(newCapacity);
    EXPECT_EQ(heap.capacity(), newCapacity);
    for (; i < newCapacity; ++i)
    {
        int value = rand();    // NOLINT(runtime/threadsafe_fn)
        Record* record = pool.New<Record>(value);
        heap.push(record);
        EXPECT_EQ(heap.capacity(), newCapacity);
    }

    // 3. Auto reserve
    Record* record = pool.New<Record>(0);
    heap.push(record);
    newCapacity *= 2;
    EXPECT_EQ(heap.capacity(), newCapacity);

    // 4. Shrink will not work
    size_t newCapacity2 = heap.capacity() / 2;
    heap.reserve(newCapacity2);
    EXPECT_NE(newCapacity2, heap.capacity());
    EXPECT_EQ(newCapacity, heap.capacity());
}

