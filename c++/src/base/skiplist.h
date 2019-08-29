#ifndef _SRC_BASE_SKIPLIST_H
#define _SRC_BASE_SKIPLIST_H

#include "src/base/atomic_pointer.h"
#include "src/common/macros.h"
#include "src/math/randomizer.h"
#include "src/memory/mempool.h"

template<typename Key, class Comparator>
class SkipList
{
private:
    struct Node;

public:
    // Create a new SkipList object that will use "cmp" for comparing keys,
    // and will allocate memory using memory pool. Objects allocated in pool
    // must remain allocated for the lifetime of the skiplist object.
    explicit SkipList(MemPool* pool);

    // Insert key into the list.
    // REQUIRES: nothing that compares equal to key is currently in the list.
    bool Insert(const Key& key);

    // Returns true if an entry that compares equal to key is in the list.
    bool Contains(const Key& key) const;

    // Iteration over the contents of a skip list
    class Iterator
    {
    public:
        // Initialize an iterator over the specified list.
        // The returned iterator is not valid.
        explicit Iterator(const SkipList* list);

        // Returns true if the iterator is positioned at a valid node.
        bool Valid() const;

        // Returns the key at the current position.
        // REQUIRES: Valid()
        const Key& key() const;

        // Advances to the next position.
        // REQUIRES: Valid()
        void Next();

        // Advances to the previous position.
        // REQUIRES: Valid()
        void Prev();

        // Advance to the first entry with a key >= target
        void Seek(const Key& target);

        // Position at the first entry in list.
        // Final state of iterator is Valid() if list is not empty.
        void SeekToFirst();

        // Position at the last entry in list.
        // Final state of iterator is Valid() if list is not empty.
        void SeekToLast();

    private:
        const SkipList* mList;
        Node* mNode;
        // Intentionally copyable
    };

private:
    Node* newHead(int height);
    Node* newNode(const Key& key, int height);

    int randomHeight();

    int getMaxHeight() const
    {
        return static_cast<int>(
            reinterpret_cast<intptr_t>(mMaxHeight.NoBarrier_Load()));
    }

    bool equal(const Key& a, const Key& b) const
    {
        return (mCompare(a, b) == 0);
    }

    // Return true if key is greater than the data stored in "n"
    bool keyIsAfterNode(const Key& key, Node* n) const;

    // Return the earliest node that comes at or after key.
    // Return NULL if there is no such node.
    //
    // If prev is non-NULL, fills prev[level] with pointer to previous
    // node at "level" for every level in [0..mMaxHeight-1].
    Node* findGreaterOrEqual(const Key& key, Node** prev) const;

    // Return the latest node with a key < key.
    // Return mHead if there is no such node.
    Node* findLessThan(const Key& key) const;

    // Return the last node in the list.
    // Return mHead if list is empty.
    Node* findLast() const;

private:
    enum { kMaxHeight = 12 };
    Comparator mCompare;
    MemPool* const mPool;
    Node* const mHead;

    // Modified only by Insert().
    // Read racily by readers, but stale values are ok.
    AtomicPointer mMaxHeight;

    // Read/written only by Insert().
    Randomizer mRand;

    DISALLOW_COPY_AND_ASSIGN(SkipList);
};

// Implementation details follow
template<typename Key, class Comparator>
struct SkipList<Key, Comparator>::Node
{
    explicit Node(const Key& k) : key(k) { }

    Key const key;

    // Accessors/mutators for links.  Wrapped in methods so we can
    // add the appropriate barriers as necessary.
    Node* Next(int n)
    {
        assert(n >= 0);
        // Use an 'acquire load' so that we observe a fully initialized
        // version of the returned Node.
        return reinterpret_cast<Node*>(mNext[n].Acquire_Load());
    }
    void SetNext(int n, Node* x)
    {
        assert(n >= 0);
        // Use a 'release store' so that anybody who reads through this
        // pointer observes a fully initialized version of the inserted node.
        mNext[n].Release_Store(x);
    }

    // No-barrier variants that can be safely used in a few locations.
    Node* NoBarrier_Next(int n)
    {
        assert(n >= 0);
        return reinterpret_cast<Node*>(mNext[n].NoBarrier_Load());
    }
    void NoBarrier_SetNext(int n, Node* x)
    {
        assert(n >= 0);
        mNext[n].NoBarrier_Store(x);
    }

private:
    // Array of length equal to the node height.  mNext[0] is lowest level link.
    AtomicPointer mNext[1];
};

template<typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node*
SkipList<Key, Comparator>::newHead(int height)
{
    Key* key = mPool->New<Key>();
    return newNode(*key, height);
}

template<typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node*
SkipList<Key, Comparator>::newNode(const Key& key, int height)
{
    void* mem = mPool->AllocAligned(
        sizeof(Node) + sizeof(AtomicPointer) * (height - 1));
    return new (mem) Node(key);
}

template<typename Key, class Comparator>
inline SkipList<Key, Comparator>::Iterator::Iterator(const SkipList* list)
{
    mList = list;
    mNode = NULL;
}

template<typename Key, class Comparator>
inline bool SkipList<Key, Comparator>::Iterator::Valid() const
{
    return mNode != NULL;
}

template<typename Key, class Comparator>
inline const Key& SkipList<Key, Comparator>::Iterator::key() const
{
    assert(Valid());
    return mNode->key;
}

template<typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::Next()
{
    assert(Valid());
    mNode = mNode->Next(0);
}

template<typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::Prev()
{
    // Instead of using explicit "prev" links, we just search for the
    // last node that falls before key.
    assert(Valid());
    mNode = mList->findLessThan(mNode->key);
    if (mNode == mList->mHead)
    {
        mNode = NULL;
    }
}

template<typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::Seek(const Key& target)
{
    mNode = mList->findGreaterOrEqual(target, NULL);
}

template<typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::SeekToFirst()
{
    mNode = mList->mHead->Next(0);
}

template<typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::SeekToLast()
{
    mNode = mList->findLast();
    if (mNode == mList->mHead)
    {
        mNode = NULL;
    }
}

template<typename Key, class Comparator>
int SkipList<Key, Comparator>::randomHeight()
{
    // Increase height with probability 1 in kBranching
    static const unsigned int kBranching = 4;
    int height = 1;
    while (height < kMaxHeight && ((mRand.Next() % kBranching) == 0))
    {
        height++;
    }
    assert(height > 0);
    assert(height <= kMaxHeight);
    return height;
}

template<typename Key, class Comparator>
bool SkipList<Key, Comparator>::keyIsAfterNode(
        const Key& key, Node* n) const
{
    // NULL n is considered infinite
    return (n != NULL) && (mCompare(n->key, key) < 0);
}

template<typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node*
SkipList<Key, Comparator>::findGreaterOrEqual(
        const Key& key, Node** prev) const
{
    Node* x = mHead;
    int level = getMaxHeight() - 1;
    while (true)
    {
        Node* next = x->Next(level);
        if (keyIsAfterNode(key, next))
        {
            // Keep searching in this list
            x = next;
        }
        else
        {
            if (prev != NULL)
            {
                prev[level] = x;
            }
            if (level == 0)
            {
                return next;
            }
            else
            {
                // Switch to next list
                level--;
            }
        }
    }
}

template<typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node*
SkipList<Key, Comparator>::findLessThan(const Key& key) const
{
    Node* x = mHead;
    int level = getMaxHeight() - 1;
    while (true)
    {
        assert(x == mHead || mCompare(x->key, key) < 0);
        Node* next = x->Next(level);
        if (next == NULL || mCompare(next->key, key) >= 0)
        {
            if (level == 0)
            {
                return x;
            } else {
                // Switch to next list
                level--;
            }
        } else {
            x = next;
        }
    }
}

template<typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node*
SkipList<Key, Comparator>::findLast() const
{
    Node* x = mHead;
    int level = getMaxHeight() - 1;
    while (true)
    {
        Node* next = x->Next(level);
        if (next == NULL)
        {
            if (level == 0)
            {
                return x;
            }
            else
            {
                // Switch to next list
                level--;
            }
        }
        else
        {
            x = next;
        }
    }
}

template<typename Key, class Comparator>
SkipList<Key, Comparator>::SkipList(MemPool* pool)
  : mPool(pool),
    mHead(newHead(kMaxHeight)),
    mMaxHeight(reinterpret_cast<void*>(1)),
    mRand(0xdeadbeef)
{
    for (int i = 0; i < kMaxHeight; i++)
    {
        mHead->SetNext(i, NULL);
    }
}

template<typename Key, class Comparator>
bool SkipList<Key, Comparator>::Insert(const Key& key)
{
    // TODO(opt): We can use a barrier-free variant of findGreaterOrEqual()
    // here since Insert() is externally synchronized.
    Node* prev[kMaxHeight];
    Node* x = findGreaterOrEqual(key, prev);

    // Our data structure does not allow duplicate insertion
    if (x && equal(key, x->key))
    {
        return false;
    }

    int height = randomHeight();
    if (height > getMaxHeight())
    {
        for (int i = getMaxHeight(); i < height; i++)
        {
            prev[i] = mHead;
        }

        // It is ok to mutate mMaxHeight without any synchronization
        // with concurrent readers.  A concurrent reader that observes
        // the new value of mMaxHeight will see either the old value of
        // new level pointers from mHead (NULL), or a new value set in
        // the loop below.  In the former case the reader will
        // immediately drop to the next level since NULL sorts after all
        // keys.  In the latter case the reader will use the new node.
        mMaxHeight.NoBarrier_Store(reinterpret_cast<void*>(height));
    }

    x = newNode(key, height);
    for (int i = 0; i < height; i++)
    {
        // NoBarrier_SetNext() suffices since we will add a barrier when
        // we publish a pointer to "x" in prev[i].
        x->NoBarrier_SetNext(i, prev[i]->NoBarrier_Next(i));
        prev[i]->SetNext(i, x);
    }

    return true;
}

template<typename Key, class Comparator>
bool SkipList<Key, Comparator>::Contains(const Key& key) const
{
    Node* x = findGreaterOrEqual(key, NULL);
    if (x != NULL && equal(key, x->key))
    {
        return true;
    }
    else
    {
        return false;
    }
}

#endif  // _SRC_BASE_SKIPLIST_H
