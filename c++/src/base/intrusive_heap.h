#ifndef _SRC_BASE_INTRUSIVE_HEAP_H
#define _SRC_BASE_INTRUSIVE_HEAP_H

#include <vector>

#include "src/common/assert.h"
#include "src/common/common.h"
#include "src/common/macro.h"

class HeapNode
{
public:
    HeapNode(): mIndex(INVALID_INDEX) {}
    bool IsSingle() const { return mIndex == INVALID_INDEX; }

private:
    template <typename T, HeapNode T::*M, typename Comparator>
    friend class IntrusiveHeap;

    size_t mIndex;
    static const size_t INVALID_INDEX = -1;

    DISALLOW_COPY_AND_ASSIGN(HeapNode);
};

/**
 * A intrusive-style heap
 *
 * IntrusiveHeap does not implement iterator to avoid invalid iterator
 * while modifying heap. So we can not iterate IntrusiveHeap by order.
 * Max/min element can be got by top() with O(1) time. And the behavior
 * is somewhat as std::vector who will auto-expand if container is full.
 *
 * NOTE: Recommend to reserve enough space once heap is created to avoid
 *       frequent memory allocation at runtime.
 *
 * @param T           type of object
 * @param M           member pointer to HeapNode
 * @param Comparator  a functor to determine order of object
 *
 * Usage:
 *   struct Record
 *   {
 *       uint64_t value;
 *       HeapNode node;
 *
 *       explicit Record(int v): value(v) { }
 *   };
 *   typedef IntrusiveHeap<
 *       Record,
 *       &Record::node> SampleHeap;
 *
 *   SampleHeap heap(1024);
 *   Record a(1);
 *   heap.push(&a);
 *   const Record& element = heap.top();
 */
template <typename T,
          HeapNode T::*M,
          typename Comparator = std::less<T> >
class IntrusiveHeap
{
    typedef HeapNode* element_type;
public:

    /**
     * Construct an empty heap whose initial capacity is zero.
     */
    explicit IntrusiveHeap(const Comparator& comparator = Comparator());

    /**
     * Construct a heap with the contents of the range [first, last).
     * And the initial capacity is length of [first, last)
     */
    template <typename InputIt>
    IntrusiveHeap(InputIt first, InputIt last,
                  const Comparator& comparator = Comparator());

    /** Insert one obj and place it in order. */
    void push(T* obj);

    /** Remove one obj and re-adjust heap. */
    void erase(T* obj);

    /** Get top object with max/min value. */
    T* top() { return mCount == 0 ? NULL : &getObj(0); }

    /** Get const top object with max/min value. */
    const T* top() const { return mCount == 0 ? NULL : &getObj(0); }

    /** Discard top object and re-adjust heap. */
    void pop() { erase(top()); }

    void reserve(size_t newCapacity) { expand(newCapacity); }

    size_t capacity() const { return mArray.capacity(); }

    size_t size() const { return mCount; }

    bool empty() const { return mCount == 0; }

    /** Remove all objects. */
    void clear()
    {
        for (size_t i = 0; i < mCount; ++i)
        {
            mArray[i]->mIndex = HeapNode::INVALID_INDEX;
            mArray[i] = NULL;
        }
        mCount = 0;
    }

private:
    void setObj(size_t index, T* obj)
    {
        HeapNode* node = &(obj->*M);
        ASSERT_DEBUG(node->IsSingle());
        mArray[index] = node;
        mArray[index]->mIndex = index;
    }

    T& getObj(size_t index) const
    {
        ASSERT_DEBUG(index < mCount);
        return *MemberToObject(mArray[index], M);
    }

    size_t getIndex(T& obj) const
    {
        size_t index = (obj.*M).mIndex;
        ASSERT_DEBUG(index < mCount && index != HeapNode::INVALID_INDEX);
        return index;
    }

    bool compare(size_t l, size_t r) const
    {
        return mComparator(getObj(l), getObj(r));
    }

    void swap(size_t l, size_t r)
    {
        HeapNode* a = mArray[l];
        HeapNode* b = mArray[r];
        mArray[r] = a;
        mArray[l] = b;
        a->mIndex = r;
        b->mIndex = l;
    }

    void shiftUp(size_t k)
    {
        size_t p = parent(k);
        while (k > 0 && compare(p, k))
        {
            swap(p, k);
            k = p;
            p = parent(k);
        }
    }

    void shiftDown(size_t k)
    {
        while (left(k) < mCount)
        {
            size_t l = left(k);
            size_t r = right(k);
            size_t m = (r < mCount && compare(l, r)) ? r : l;
            if (compare(m, k)) return;
            swap(m, k);
            k = m;
        }
    }

    void expand(size_t newCapacity)
    {
        // Use resize() rather than reserve(), for that reserve
        // will not move elements once reallocate memory
        if (newCapacity > mArray.capacity())
        {
            mArray.resize(newCapacity, NULL);
            ASSERT_DEBUG(mArray.capacity() == newCapacity);
        }
    }

    void buildHeap()
    {
        int64_t index = static_cast<int64_t>(mCount / 2);
        while (--index >= 0) { shiftDown(index); }
    }

    static size_t left(size_t i) { return 2 * i + 1; }
    static size_t right(size_t i) { return 2 * i + 2; }
    static size_t parent(size_t i) { return (i - 1) / 2; }

    std::vector<element_type> mArray;
    size_t mCount;
    const Comparator mComparator;

    DISALLOW_COPY_AND_ASSIGN(IntrusiveHeap);
};

template <typename T,
          HeapNode T::*M,
          typename Comparator>
IntrusiveHeap<T, M, Comparator>::
IntrusiveHeap(const Comparator& comparator)
    : mCount(0),
      mComparator(comparator)
{
}

template <typename T,
          HeapNode T::*M,
          typename Comparator>
template <typename InputIt>
IntrusiveHeap<T, M, Comparator>::
IntrusiveHeap(InputIt first, InputIt last, const Comparator& comparator)
    : mCount(0),
      mComparator(comparator)
{
    size_t capacity = 0;
    for (InputIt iter = first; iter != last; ++iter)
    {
        ++capacity;
    }
    expand(capacity);

    for (InputIt iter = first; iter != last; ++iter)
    {
        mArray[mCount++] = &((*iter).*M);
    }
    buildHeap();
}

template <typename T,
          HeapNode T::*M,
          typename Comparator>
inline void IntrusiveHeap<T, M, Comparator>::
    push(T* obj)
{
    if (UNLIKELY(mCount >= mArray.capacity()))
    {
        // Initialize 4K memory (512 * sizeof(HeapNode*))) on first push
        ASSERT_DEBUG(mCount == mArray.capacity());
        size_t oldCapacity = mArray.capacity();
        size_t newCapacity = oldCapacity == 0 ? 512 : oldCapacity * 2;
        expand(newCapacity);
    }
    setObj(mCount, obj);
    shiftUp(mCount++);
}

template <typename T,
          HeapNode T::*M,
          typename Comparator>
inline void IntrusiveHeap<T, M, Comparator>::
    erase(T* obj)
{
    HeapNode* node = &(obj->*M);
    size_t index = node->mIndex;
    ASSERT_DEBUG(index != HeapNode::INVALID_INDEX);
    ASSERT_DEBUG(mCount > index);
    swap(index, mCount - 1);    // Avoid judging the last object
    shiftUp(index);
    --mCount;
    mArray[mCount] = NULL;
    node->mIndex = HeapNode::INVALID_INDEX;
    shiftDown(index);
}

#endif // _SRC_BASE_INTRUSIVE_HEAP_H
