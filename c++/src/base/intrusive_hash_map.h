#ifndef _SRC_BASE_INTRUSIVE_HASH_MAP_H
#define _SRC_BASE_INTRUSIVE_HASH_MAP_H

// As we need long template parameters, it affects code readability if we
// limit line length to 80 characters.

#include <utility>
#include <tr1/functional>
#include "src/base/intrusive_list.h"
#include "src/common/assert.h"
#include "src/common/macro.h"
#include "src/math/math.h"

/**
 * A intrusive-style hash table with fixed number of buckets.
 *
 * @param Key         type of Key
 * @param Value       type of Value
 * @param KeyMember   member pointer to Key
 * @param LinkMember  member pointer to LinkNode
 * @param Hash        a hash function defined on Key
 * @param Equal       a functor that tests if two objects of type Key are equal
 *
 * Usage:
 *   struct Record
 *   {
 *       uint64_t key;
 *       uint64_t value;
 *       LinkNode node;
 *   };
 *   typedef IntrusiveHashMap<
 *       uint64_t,       // Key
 *       Record,         // Value
 *       &Record::key,
 *       &Record::node> MyHashMap;
 *
 *   MyHashMap map;
 *   Record a;
 *   a.key = 1;
 *   a.value = 2;
 *   map.insert(&a);
 *
 * Caveats:
 * - Iteration over the container is O(B) where B is the number of buckets.
 * - Must NOT call LinkNode::Unlink.
 * - The number of buckets are always power of two. Therefore the hash function
 *   (on the given key set) must be able to produce hash values with distinct
 *   least bits on different inputs in most cases.  For example, Self
 *   incremental IDs are good with std::hash<int>.  IPv4 addresses are bad, as
 *   most addresses in our production systems are "10.x.y.z" or "11.x.y.z".
 */
template<typename Key,
         typename Value,
         Key Value::*KeyMember,
         LinkNode Value::*LinkMember,
         typename Hash = std::tr1::hash<Key>,
         typename Equal = std::equal_to<Key> >
class IntrusiveHashMap
{
public:
    class iterator;
    class const_iterator;
    typedef IntrusiveHashMap self;
    typedef Value element_type;

    /**
     * Initialize the hash map with a fixed number of buckets. "bucketSize" will
     * be rounded up to power of two internally to stay away from the expensive
     * MOD operation.
     */
    explicit IntrusiveHashMap(size_t bucketSize,
                              const Hash& hash = Hash(),
                              const Equal& equal = Equal());

    ~IntrusiveHashMap()
    {
        Value* dummyValue = reinterpret_cast<Value*>(&mDummyValue[0]);
        (dummyValue->*LinkMember).~LinkNode();
        delete[] mBuckets;
    }

    ///////////////
    // Iterators
    iterator begin()
    {
        mFirstNonEmptyBucket = seekToFirstNonEmptyBucket();
        return iterator(mFirstNonEmptyBucket);
    }

    const_iterator begin() const { return const_cast<self*>(this)->begin(); }

    iterator end() { return iterator(&mBuckets[mBucketSize]); }

    const_iterator end() const { return const_cast<self*>(this)->end(); }

    //////////////
    // Capacity
    bool empty() const { return mSize == 0; }
    size_t size() const { return mSize; }

    ///////////////////
    // Element access

    /**
     * Find a key in this container.
     *
     * @param key  the key to find
     * @return     the iterator pointing to the given key, or end() if not found
     */
    iterator find(const Key& key);

    /**
     * The const-version of find().
     */
    const_iterator find(const Key& key) const
    {
        return const_cast<self*>(this)->find(key);
    }

    ///////////////
    // Modifiers
    std::pair<iterator, bool> insert(Value* value);

    void erase(iterator pos);

    void erase(Value* value);

    /**
     * Remove a entries with specified key.
     *
     * @return 0 if key does not exist, or 1 if the deletion is taken
     *           successfully.
     */
    size_t erase(const Key& key)
    {
        iterator i = find(key);
        if (i == end())
        {
            return 0;
        }
        erase(i);
        return 1;
    }

    void clear()
    {
        Bucket* bucket = seekToFirstNonEmptyBucket();
        for ( ; bucket != &mBuckets[mBucketSize]; bucket++)
        {
            bucket->clear();
        }
        mSize = 0;
        mFirstNonEmptyBucket = &mBuckets[mBucketSize];
    }

private:
    typedef IntrusiveList<Value, LinkMember> Bucket;

    Bucket* findBucket(const Key& key)
    {
        return &mBuckets[mHash(key) & (mBucketSize - 1)];
    }

    const Bucket* findBucket(const Key& key) const
    {
        return const_cast<self*>(this)->findBucket(key);
    }

    const Key& keyOf(const Value& value) const { return value.*KeyMember; }

    void moveNext(iterator* iterator);

    Bucket* seekToFirstNonEmptyBucket()
    {
        if (mFirstNonEmptyBucket != &mBuckets[mBucketSize]) {
            return mFirstNonEmptyBucket;
        } else if (empty()) {
            return &mBuckets[mBucketSize];
        } else {
            Bucket* bucket = &mBuckets[0];
            while (bucket->empty())
            {
                bucket++;
            }
            return bucket;
        }
    }

    Bucket *mBuckets;
    size_t mBucketSize;
    size_t mSize;
    Hash   mHash;
    Equal  mEqual;

    /**
     * Cache the pointer to first non-empty bucket, so that begin() is O(1).
     * It may point to mBuckets[mBucketSize] if the container is empty.
     */
    Bucket* mFirstNonEmptyBucket;
    char mDummyValue[sizeof(Value)];

    DISALLOW_COPY_AND_ASSIGN(IntrusiveHashMap);
};

template<typename Key,
         typename Value,
         Key Value::*KeyMember,
         LinkNode Value::*LinkMember,
         typename Hash,
         typename Equal>
class IntrusiveHashMap<Key, Value, KeyMember, LinkMember, Hash, Equal>::iterator
{
public:
    typedef iterator self;
    typedef Value value_type;
    typedef ptrdiff_t difference_type;
    typedef Value* pointer;
    typedef Value& reference;
    typedef std::forward_iterator_tag iterator_category;

    iterator() : mBucket(NULL) {}

    explicit iterator(Bucket* b)
        : mIterator(b->begin()), mBucket(b)
    {
        ASSERT_DEBUG(!b->empty());
    }

    iterator(Bucket* b, typename Bucket::iterator i)
        : mIterator(i), mBucket(b)
    {
        ASSERT_DEBUG(!b->empty());
    }

    self& operator++()
    {
        ASSERT_DEBUG(mIterator != mBucket->end());
        ++mIterator;
        while (mIterator == mBucket->end())
        {
            mBucket++;
            mIterator = mBucket->begin();
        }
        return *this;
    }

    self operator++(int)
    {
        self copy = *this;
        operator++();
        return copy;
    }

    pointer operator->() { return get(); }
    reference operator*() { return *get(); }

    bool operator==(const self& right) const
    {
        return mIterator == right.mIterator && mBucket == right.mBucket;
    }

    bool operator!=(const self& right) const { return !operator==(right); }

private:
    friend class IntrusiveHashMap;

    Value* get() { return &(*mIterator); }

    typename Bucket::iterator mIterator;
    Bucket* mBucket;
};

template<typename Key,
         typename Value,
         Key Value::*KeyMember,
         LinkNode Value::*LinkMember,
         typename Hash,
         typename Equal>
class IntrusiveHashMap<Key, Value, KeyMember, LinkMember, Hash, Equal>::
    const_iterator
{
public:
    typedef const_iterator self;
    typedef Value value_type;
    typedef ptrdiff_t difference_type;
    typedef const Value* pointer;
    typedef const Value& reference;
    typedef std::forward_iterator_tag iterator_category;

    const_iterator() {}
    /* implicit */ const_iterator(typename IntrusiveHashMap::iterator i)
        : mIterator(i)
    {
    }

    self& operator++()
    {
        ++mIterator;
        return *this;
    }

    self& operator++(int)
    {
        self copy = *this;
        ++mIterator;
        return copy;
    }

    pointer operator->() { return &(*mIterator); }

    reference operator*() { return *mIterator; }

    bool operator==(const self& right) const
    {
        return mIterator == right.mIterator;
    }

    bool operator!=(const self& right) const
    {
        return !operator==(right);
    }

private:
    typename IntrusiveHashMap::iterator mIterator;  // hold a non-const iterator
};

// IMPLEMENTATION OF IntrusiveHashMap
template<typename Key,
         typename Value,
         Key Value::*KeyMember,
         LinkNode Value::*LinkMember,
         typename Hash,
         typename Equal>
IntrusiveHashMap<Key, Value, KeyMember, LinkMember, Hash, Equal>::
IntrusiveHashMap(size_t bucketSize,
                 const Hash& hash,
                 const Equal& equal)
    : mSize(0),
      mHash(hash),
      mEqual(equal)
{
    bucketSize = RoundUpToPowerOfTwo(bucketSize);

    // Allocate one more bucket and insert a dummy value.  This help to save
    // null-checks in begin() and iterator::operator++().
    mBuckets = new Bucket[bucketSize + 1];
    mBucketSize = bucketSize;

    // Construct a dummy value.  Take care that we do not initialize Value here,
    // as it may have no (accessible) zero-argument constructor.
    Bucket* last = &mBuckets[bucketSize];
    Value* dummyValue = reinterpret_cast<Value*>(&mDummyValue[0]);
    new (&(dummyValue->*LinkMember)) LinkNode;
    last->push_back(dummyValue);

    mFirstNonEmptyBucket = last;
}

template<typename Key,
         typename Value,
         Key Value::*KeyMember,
         LinkNode Value::*LinkMember,
         typename Hash,
         typename Equal>
typename
IntrusiveHashMap<Key, Value, KeyMember, LinkMember, Hash, Equal>::iterator
IntrusiveHashMap<Key, Value, KeyMember, LinkMember, Hash, Equal>::
    find(const Key& key)
{
    Bucket* bucket = findBucket(key);
    FOREACH(i, *bucket)
    {
        if (mEqual(keyOf(*i), key))
        {
            return iterator(bucket, i);
        }
    }
    return end();
}

template<typename Key,
         typename Value,
         Key Value::*KeyMember,
         LinkNode Value::*LinkMember,
         typename Hash,
         typename Equal>
std::pair<typename // NOLINT(whitespace/operators)
    IntrusiveHashMap<Key, Value, KeyMember, LinkMember, Hash, Equal>::iterator,
    bool>
IntrusiveHashMap<Key, Value, KeyMember, LinkMember, Hash, Equal>::
    insert(Value* value)
{
    Bucket* bucket = findBucket(keyOf(*value));
    FOREACH(i, *bucket)
    {
        if (mEqual(keyOf(*i), keyOf(*value)))
        {
            // Found existing key.
            return std::make_pair(iterator(bucket, i), false);
        }
    }
    typename Bucket::iterator j = bucket->insert(bucket->end(), value);
    mSize++;
    if (UNLIKELY(mSize == 1u))
    {
        mFirstNonEmptyBucket = bucket;
    }
    else if (UNLIKELY(mFirstNonEmptyBucket != &mBuckets[mBucketSize] &&
            bucket < mFirstNonEmptyBucket))
    {
        mFirstNonEmptyBucket = bucket;
    }
    return std::make_pair(iterator(bucket, j), true);
}

template<typename Key,
         typename Value,
         Key Value::*KeyMember,
         LinkNode Value::*LinkMember,
         typename Hash,
         typename Equal>
void IntrusiveHashMap<Key, Value, KeyMember, LinkMember, Hash, Equal>::
    erase(Value* value)
{
    (value->*LinkMember).Unlink();
    mSize--;
    if (mFirstNonEmptyBucket->empty())
    {
        mFirstNonEmptyBucket = &mBuckets[mBucketSize];
    }
}

template<typename Key,
         typename Value,
         Key Value::*KeyMember,
         LinkNode Value::*LinkMember,
         typename Hash,
         typename Equal>
void IntrusiveHashMap<Key, Value, KeyMember, LinkMember, Hash, Equal>::
    erase(iterator pos)
{
    ASSERT_DEBUG(pos != end());
    Value* value = &(*pos);
    erase(value);
}

#endif  // _SRC_BASE_INTRUSIVE_HASH_MAP_H
