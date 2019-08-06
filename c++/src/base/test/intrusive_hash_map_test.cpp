#include "src/base/intrusive_hash_map.h"

#include <gtest/gtest.h>

struct Record
{
    int key;
    int value;
    LinkNode node;

    Record() : key(0), value(0) {}
    Record(int _key, int _value) : key(_key), value(_value) {}
};
typedef IntrusiveHashMap<int, Record, &Record::key, &Record::node> HashMap;

struct CompositeKey
{
    uint64_t a;
    uint64_t b;

    CompositeKey(int _a, int _b) : a(_a), b(_b) {}
};

struct CompositeRecord
{
    CompositeKey key;
    int value;
    LinkNode node;

    CompositeRecord(int key1, int key2, int _value)
        : key(key1, key2), value(_value)
    {
    }
};

class CompositeKeyHash
{
public:
    size_t operator()(const CompositeKey& key)
    {
        return 31 * key.a + key.b;
    }
};

class CompositeKeyEqual
{
public:
    bool operator()(const CompositeKey& left, const CompositeKey& right)
    {
        return left.a == right.a && left.b == right.b;
    }
};

typedef IntrusiveHashMap<
    CompositeKey,
    CompositeRecord,
    &CompositeRecord::key,
    &CompositeRecord::node,
    CompositeKeyHash,
    CompositeKeyEqual> CompositeHashMap;

TEST(IntrusiveHashMap, Find)
{
    HashMap map(4);
    Record a(1, 100);
    Record b(2, 200);
    Record c(5, 500);  // collision with "1"
    map.insert(&a);
    map.insert(&b);
    map.insert(&c);
    HashMap::iterator i = map.find(1);
    EXPECT_EQ(1, i->key);
    EXPECT_EQ(100, i->value);
    HashMap::iterator j = map.find(2);
    EXPECT_EQ(2, j->key);
    EXPECT_EQ(200, j->value);
    HashMap::iterator k = map.find(3);
    EXPECT_EQ(k, map.end());
    HashMap::iterator l = map.find(5);
    EXPECT_EQ(500, l->value);
}

TEST(IntrusiveHashMap, Insert)
{
    HashMap map(2);
    Record a(1, 100);
    Record b(1, 200);
    std::pair<HashMap::iterator, bool> i = map.insert(&a);
    EXPECT_EQ(1, i.first->key);
    EXPECT_EQ(100, i.first->value);
    EXPECT_TRUE(i.second);
    std::pair<HashMap::iterator, bool> j = map.insert(&b);
    EXPECT_EQ(100, j.first->value);
    EXPECT_FALSE(j.second);
}

TEST(IntrusiveHashMap, Iterator)
{
    const int M = 4;
    const int N = 10;
    HashMap map(M);
    Record a[N];
    for (int i = 0; i < N; i++)
    {
        a[i].key = i;
        a[i].value = i * 100;
        map.insert(&a[i]);
    }
    EXPECT_EQ(10U, map.size());

    // non-const iterator
    int sum = 0;
    int count = 0;
    FOREACH(i, map)
    {
        sum += i->value;
        count++;
    }
    EXPECT_EQ(4500, sum);
    EXPECT_EQ(10, count);

    // const iterator
    sum = 0;
    count = 0;
    FOREACH(i, *const_cast<const HashMap*>(&map))
    {
        sum += i->value;
        count++;
    }
    EXPECT_EQ(4500, sum);
    EXPECT_EQ(10, count);
}

TEST(IntrusiveHashMap, Clear)
{
    HashMap map(2);
    Record a(1, 100);
    Record b(2, 200);
    Record c(3, 300);
    map.insert(&a);
    map.insert(&b);
    EXPECT_EQ(2U, map.size());
    map.clear();
    EXPECT_EQ(0U, map.size());
    map.insert(&c);
    EXPECT_EQ(1U, map.size());
    EXPECT_EQ(300, map.find(3)->value);
    EXPECT_EQ(map.end(), map.find(1));
    EXPECT_EQ(map.end(), map.find(2));
}

TEST(IntrusiveHashMap, EraseKey)
{
    HashMap map(2);
    Record a(1, 100);
    Record b(2, 200);
    map.insert(&a);
    map.insert(&b);
    EXPECT_EQ(2U, map.size());
    EXPECT_EQ(0U, map.erase(3));
    EXPECT_EQ(2U, map.size());
    EXPECT_EQ(1U, map.erase(1));
    EXPECT_EQ(1U, map.size());
    EXPECT_EQ(map.end(), map.find(1));
    EXPECT_EQ(200, map.find(2)->value);
}

TEST(IntrusiveHashMap, EraseIterator)
{
    HashMap map(2);
    Record a(1, 100);
    map.insert(&a);
    HashMap::iterator i = map.find(1);
    EXPECT_NE(map.end(), i);
    map.erase(i);
    EXPECT_EQ(map.end(), map.find(1));
}

TEST(IntrusiveHashMap, HashFnEqualFn)
{
    CompositeHashMap map(2);
    CompositeRecord a(1, 10, 100);
    CompositeRecord b(2, 20, 200);
    CompositeRecord c(3, 30, 300);
    map.insert(&a);
    map.insert(&b);
    map.insert(&c);
    CompositeHashMap::iterator i = map.find(CompositeKey(1, 10));
    CompositeHashMap::iterator j = map.find(CompositeKey(3, 30));
    EXPECT_EQ(100, i->value);
    EXPECT_EQ(300, j->value);
    CompositeHashMap::iterator k = map.find(CompositeKey(1, 30));
    EXPECT_EQ(map.end(), k);
}
