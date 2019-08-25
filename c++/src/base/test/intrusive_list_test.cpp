#include <gtest/gtest.h>
#include <algorithm>

#include "src/base/intrusive_list.h"
#include "src/common/macros.h"

#define EXPECT_LIST_EQ(list, ...)                         \
    do                                                    \
    {                                                     \
        int __answer[] = { __VA_ARGS__ };                 \
        size_t __answer_len = COUNT_OF(__answer);   \
        EXPECT_EQ((list).size(), __answer_len);           \
        ListType::const_iterator __i;                     \
        size_t __k;                                       \
        for (__i = (list).begin(), __k = 0;               \
             __i != (list).end() && __k < __answer_len;   \
             ++__i, ++__k)                                \
        {                                                 \
            EXPECT_EQ(__i->value, __answer[__k]);         \
        }                                                 \
    } while (0)

struct Bar
{
    int value;
    LinkNode node;
};

typedef IntrusiveList<Bar, &Bar::node> ListType;

template <typename Iterator>
Iterator MoveForward(Iterator i, size_t n);

TEST(IntrusiveListTest, ListTest)
{
    ListType l;
    EXPECT_EQ(l.size(), 0UL);
    EXPECT_TRUE(l.empty());

    const int COUNT = 1000;
    Bar bars[COUNT];
    for (int k = 0; k < COUNT; ++k) {
        Bar *bar = &bars[k];
        bar->value = k;
        l.push_back(bar);
    }
    EXPECT_EQ(l.size(), static_cast<size_t>(COUNT));

    ListType::iterator it = l.begin();
    for (int k = 0; k < COUNT; ++k, ++it) {
        EXPECT_EQ(it->value, k);
    }
    EXPECT_TRUE(it == l.end());

    EXPECT_EQ(0, l.front()->value);
    EXPECT_EQ(COUNT - 1, l.back()->value);

    for (int k = 0; k < COUNT; ++k) {
        Bar *bar = l.front();
        l.pop_front();
        EXPECT_EQ(bar->value, k);
    }

    EXPECT_EQ(l.size(), 0UL);
    EXPECT_TRUE(l.empty());

    for (int k = 0; k < COUNT; ++k) {
        l.push_back(&bars[k]);
    }

    l.clear();

    EXPECT_EQ(l.size(), 0UL);
    EXPECT_TRUE(l.empty());
}

TEST(IntrusiveListTest, Splice)
{
    ListType a;
    ListType b;
    Bar e[5];
    for (int i = 0; i < 5; i++)
    {
        e[i].value = i;
        a.push_back(&e[i]);
    }

    // splice from another list
    b.splice(b.end(), a.begin(), MoveForward(a.begin(), 2));
    EXPECT_LIST_EQ(a, 2, 3, 4);
    EXPECT_LIST_EQ(b, 0, 1);
    b.splice(b.end(), a.begin(), MoveForward(a.begin(), 1));
    EXPECT_LIST_EQ(a, 3, 4);
    EXPECT_LIST_EQ(b, 0, 1, 2);
    b.splice(MoveForward(b.begin(), 2), a.begin(), a.end());
    EXPECT_LIST_EQ(a);
    EXPECT_LIST_EQ(b, 0, 1, 3, 4, 2);
    a.splice(a.begin(), MoveForward(b.begin(), 1), MoveForward(b.begin(), 3));
    EXPECT_LIST_EQ(a, 1, 3);
    EXPECT_LIST_EQ(b, 0, 4, 2);
    a.splice(a.end(), b.begin(), b.begin());
    a.splice(a.end(), b.end(), b.end());
    EXPECT_LIST_EQ(a, 1, 3);
    EXPECT_LIST_EQ(b, 0, 4, 2);
    a.splice(a.begin(), b.begin());
    a.splice(a.end(), b.begin());
    a.splice(MoveForward(a.begin(), 2), b.begin());
    EXPECT_LIST_EQ(a, 0, 1, 2, 3, 4);
    EXPECT_LIST_EQ(b);

    // splice from self
    //
    // Note that: a.splice(a.begin(), a.begin(), a.end()) is illegal
    // according to [1]. as "position" should not be within the range
    // of [first, last).
    // [1]: http://en.cppreference.com/w/cpp/container/list/splice
    a.splice(a.end(), a.begin(), a.end());
    EXPECT_LIST_EQ(a, 0, 1, 2, 3, 4);
    a.splice(a.end(), a.begin(), MoveForward(a.begin(), 2));
    EXPECT_LIST_EQ(a, 2, 3, 4, 0, 1);
    a.splice(MoveForward(a.begin(), 2), MoveForward(a.begin(), 3), a.end());
    EXPECT_LIST_EQ(a, 2, 3, 0, 1, 4);
    a.splice(a.begin(), MoveForward(a.begin(), 2), MoveForward(a.begin(), 4));
    EXPECT_LIST_EQ(a, 0, 1, 2, 3, 4);
}

TEST(IntrusiveListTest, Swap)
{
    ListType a;
    ListType b;
    Bar e[5];
    for (int i = 0; i < 5; i++)
    {
        e[i].value = i;
        a.push_back(&e[i]);
    }
    a.swap(&b);
    EXPECT_LIST_EQ(b, 0, 1, 2, 3, 4);
    EXPECT_TRUE(a.empty());
}

TEST(IntrusiveListTest, Erase)
{
    ListType a;
    Bar e[10];
    for (int i = 0; i < 10; i++)
    {
        e[i].value = i;
        a.push_back(&e[i]);
    }
    decltype(a.begin()) it = a.begin();
    std::advance(it, 5);
    a.erase(it, it);
    EXPECT_LIST_EQ(a, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    decltype(a.begin()) eit = it;
    ++eit;
    a.erase(it, eit);
    EXPECT_LIST_EQ(a, 0, 1, 2, 3, 4, 6, 7, 8, 9);
    a.erase(a.begin(), eit);
    EXPECT_LIST_EQ(a, 6, 7, 8, 9);
    a.erase(a.begin(), a.end());
    EXPECT_TRUE(a.empty());
}

TEST(IntrusiveListTest, Link)
{
    ListType a;
    Bar e[10];
    for (int i = 0; i < 10; i++)
    {
        e[i].value = i;
        a.push_back(&e[i]);
    }
    decltype(a.begin()) it = a.begin();
    std::advance(it, 5);
    decltype(a.begin()) eit = it;
    ++eit;
    a.link(it, eit);
    EXPECT_LIST_EQ(a, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    ++eit;
    a.link(it, eit);
    EXPECT_LIST_EQ(a, 0, 1, 2, 3, 4, 5, 7, 8, 9);
    a.link(a.begin(), it);
    EXPECT_LIST_EQ(a, 0, 5, 7, 8, 9);
    a.link(a.end(), it);
    EXPECT_LIST_EQ(a, 5, 7, 8, 9);
    a.link(eit, a.end());
    EXPECT_LIST_EQ(a, 5, 7);
    a.link(a.end(), a.end());
    EXPECT_TRUE(a.empty());
}

template <typename Iterator>
Iterator MoveForward(Iterator i, size_t n)
{
    for (size_t k = 0; k < n; k++)
    {
        ++i;
    }
    return i;
}


#if 0
TEST(IntrusiveListTest, DListTest)
{
    DListType l;
    EXPECT_EQ(l.size(), 0UL);
    EXPECT_TRUE(l.empty());

    const int count = 1000;
    for (int k = 0; k < count; ++k) {
        Foo *foo = new Foo;
        foo->value = k;
        l.push_back(*foo);
    }
    EXPECT_EQ(l.size(), (size_t)count);

    DListType::iterator it = l.begin();
    for (int k = 0; k < count; ++k, ++it) {
        EXPECT_EQ(it->value, k);
    }
    EXPECT_TRUE(it == l.end());

    EXPECT_EQ(0, l.front().value);
    EXPECT_EQ(count - 1, l.back().value);

    Foo *foo = &l.back();
    l.pop_back();
    delete foo;

    EXPECT_EQ(l.size(), (size_t)count - 1);

    DListType::reverse_iterator rit = l.rbegin();
    for (int k = count - 1; k > 0; --k, ++rit) {
        EXPECT_EQ(rit->value, k - 1);
    }
    EXPECT_TRUE(rit == l.rend());

    l.erase(l.begin());
    EXPECT_EQ(1, l.front().value);

    l.clear();
    EXPECT_EQ(l.size(), 0UL);
    EXPECT_TRUE(l.empty());
}

TEST(IntrusiveListTest, DSwapTest)
{
    DListType left, right;

    const int count = 10;
    for (int k = 0; k < count; ++k) {
        Foo *foo = new Foo;
        foo->value = k;
        left.push_back(*foo);
    }
    std::swap(left, right);
    EXPECT_EQ(left.size(), 0UL);
    EXPECT_EQ(right.size(), (size_t)count);

    DListType::reverse_iterator rit = right.rbegin();
    for (int k = count; k > 0; --k, ++rit) {
        EXPECT_EQ(rit->value, k - 1);
    }
    EXPECT_TRUE(rit == right.rend());
}

TEST(IntrusiveListTest, SwapTest)
{
    ListType left, right;

    const int count = 10;
    for (int k = 0; k < count; ++k) {
        Bar *bar = new Bar;
        bar->value = k;
        left.push_back(*bar);
    }
    std::swap(left, right);
    EXPECT_EQ(left.size(), 0UL);
    EXPECT_EQ(right.size(), (size_t)count);

    ListType::iterator it = right.begin();
    for (int k = 0; k < count; ++k, ++it) {
        EXPECT_EQ(it->value, k);
    }
    EXPECT_TRUE(it == right.end());
}
#endif  // 0
