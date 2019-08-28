#include <gtest/gtest.h>

#include "src/base/skiplist.h"

struct Record
{
    int64_t value;
    Record(): value(0) { }
    explicit Record(int64_t v): value(v) { }
};

struct RecordCompare
{
    int64_t operator()(const Record& left, const Record& right) const
    {
        return left.value - right.value;
    }
};

typedef SkipList<Record, RecordCompare> MySkipList;
typedef SkipList<Record, RecordCompare>::Iterator MyIterator;

TEST(SkipListTest, Insert)
{
    MemPool pool;
    MySkipList* list = new MySkipList(&pool);

    size_t rounds = 100000;
    for (size_t i = 0; i < rounds; ++i)
    {
        Record record(i);
        EXPECT_TRUE(list->Insert(record));
    }
}

TEST(SkipListTest, Contain)
{
    MemPool pool;
    MySkipList list(&pool);
    Randomizer rander(unsigned(time(NULL)));

    size_t rounds = 100000;
    for (size_t i = 0; i < rounds; ++i)
    {
        Record record(rander.Next());
        if (list.Insert(record))
        {
            EXPECT_TRUE(list.Contains(record));
        }
    }
}

TEST(SkipListTest, CheckSequence)
{
    MemPool pool;
    MySkipList list(&pool);
    Randomizer rander(unsigned(time(NULL)));

    size_t rounds = 100000;
    for (size_t i = 0; i < rounds; ++i)
    {
        Record record(rander.RandUInt32());
        list.Insert(record);
    }

    RecordCompare comp;
    MyIterator it(&list);
    it.SeekToFirst();
    const Record* prev = &(it.key());
    it.Next();
    while (it.Valid())
    {
        EXPECT_TRUE(comp(*prev, it.key()) < 0);
        EXPECT_LT(prev->value, it.key().value);
        prev = &(it.key());
        it.Next();
    }
}
