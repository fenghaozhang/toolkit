#include <gtest/gtest.h>

#include "src/base/bit_map.h"
#include "src/math/randomizer.h"

TEST(BitMap, Capacity)
{
    BitMap map(1023);
    EXPECT_EQ(map.Capacity(), 1023);
}

TEST(BitMap, Get)
{
    BitMap map(1024);
    EXPECT_FALSE(map.Get(0));
    EXPECT_FALSE(map[1023]);
}

TEST(BitMap, Set)
{
    BitMap map(63);
    map.Set(3);
    map.Set(33);
    EXPECT_TRUE(map.Get(3));
    EXPECT_TRUE(map.Get(33));
    EXPECT_FALSE(map.Get(2));
    EXPECT_FALSE(map.Get(32));
}

TEST(BitMap, Clear)
{
    BitMap map(1023);
    map.Set(3);
    map.Set(33);
    EXPECT_TRUE(map[3]);
    EXPECT_TRUE(map[33]);
    map.Clear(3);
    map.Clear(33);
    EXPECT_FALSE(map[3]);
    EXPECT_FALSE(map[33]);
}

TEST(BitMap, Reset)
{
    BitMap map(1023);
    for (size_t i = 0; i < 1023; ++i)
    {
        map.Set(i);
    }
    for (size_t i = 0; i < 1023; ++i)
    {
        EXPECT_TRUE(map[i]);
    }
    map.Reset();
    for (size_t i = 0; i < 1023; ++i)
    {
        EXPECT_FALSE(map[i]);
    }
}

TEST(BitMap, Count)
{
    BitMap map(1023);
    for (size_t i = 0; i < 1023; ++i)
    {
        map.Set(i);
    }
    EXPECT_EQ(map.Count(), 1023);
    for (size_t i = 0; i < 100; ++i)
    {
        map.Clear(i);
    }
    EXPECT_EQ(map.Count(), 1023 - 100);
}

TEST(SparseBitMap, Set1)
{
    SparseBitMap bitmap(1000);
    std::vector<uint32_t> bitVec;

    bitVec.push_back(0U);
    bitVec.push_back(4U);
    bitVec.push_back(8U);
    bitVec.push_back(2047U);

    for (size_t i = 0; i < bitVec.size(); i++)
    {
        bitmap.Set(bitVec[i]);
    }

    for (size_t i = 0; i < bitmap.Capacity(); i++)
    {
        size_t j = 0;
        for (j = 0; j < bitVec.size(); j++)
        {
            if (bitVec[j] == i)
            {
                EXPECT_TRUE(bitmap.Get(i));
                break;
            }
        }
        if (j == bitVec.size())
        {
            EXPECT_FALSE(bitmap.Get(i));
        }
    }
}

TEST(SparseBitMap, Get)
{
    SparseBitMap bitmap;
    size_t size = 1000000;
    for (size_t i = size; i > 0; i -= 10)
    {
        bitmap.Set(i);
    }
    for (size_t i = size; i > 0; i -= 10)
    {
        EXPECT_TRUE(bitmap[i]);
    }
}

TEST(SparseBitMap, Clear)
{
    SparseBitMap bitmap;
    int size = 1000000;
    for (int i = size; i > 0; i -= 11)
    {
        bitmap.Set(i);
    }
    for (int i = size; i > 0; i -= 11)
    {
        bitmap.Clear(i);
    }
    for (int i = size; i > 0; i -= 11)
    {
        EXPECT_FALSE(bitmap[i]);
    }
}

TEST(SparseBitMap, Reset)
{
    SparseBitMap bitmap;
    int size = 1000000;
    for (int i = size; i > 0; i -= 10)
    {
        bitmap.Set(i);
    }
    bitmap.Reset();
    for (int i = size; i > 0; --i)
    {
        EXPECT_FALSE(bitmap[i]);
    }
}

TEST(SparseBitMap, Capacity)
{
    SparseBitMap bitmap(1023);
    EXPECT_EQ(bitmap.Capacity(), 1023);
    bitmap.Set(102355);
    EXPECT_EQ(bitmap.Capacity(), 102356);
    bitmap.Clear(102355);
    EXPECT_EQ(bitmap.Capacity(), 102356);
}

TEST(SparseBitMap, Arena)
{
    Arena arena;
    SparseBitMap bitmap(0, &arena);
    int size = 10000;
    for (int i = size; i > 0; i -= 2)
    {
        bitmap.Set(i);
    }
    size += 100000;
    for (int i = size; i > 0; i -= 3)
    {
        bitmap.Set(i);
    }
    size += 1000000;
    for (int i = size; i > 0; i -= 4)
    {
        bitmap.Set(i);
    }
    size += 10000000;
    for (int i = size; i > 0; i -= 5)
    {
        bitmap.Set(i);
    }
    for (size_t i = 0; i < bitmap.Capacity(); ++i)
    {
        bitmap.Clear(i);
    }
    for (size_t i = 0; i < bitmap.Capacity(); ++i)
    {
        EXPECT_FALSE(bitmap[i]);
    }
}

TEST(SparseBitMap, RandTest)
{
    Randomizer rander((unsigned)(time(NULL)));
    SparseBitMap bitmap(0);
    size_t round = 10000000;
    for (size_t i = 0; i < round; ++i)
    {
        bitmap.Set(rander.Rand(round));
        bitmap.Get(rander.Rand(round));
        bitmap.Clear(rander.Rand(round));
    }
    bitmap.Reset();

    Arena arena;
    SparseBitMap bitmap2(0, &arena);
    for (size_t i = 0; i < round; ++i)
    {
        bitmap2.Set(rander.Rand(round));
        bitmap2.Get(rander.Rand(round));
        bitmap2.Clear(rander.Rand(round));
    }
    bitmap2.Reset();
}
