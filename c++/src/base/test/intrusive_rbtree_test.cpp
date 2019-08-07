#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include "src/base/intrusive_map.h"
#include "src/base/intrusive_rbtree.h"

struct TestNode
{
    int key;
    int value1;
    double value2;
    MapLinkNode node;

    TestNode()
        : key(0),
          value1(0),
          value2(0)
    {
    }
};

struct TestNodeLess
{
    bool operator()(const TestNode* a, const TestNode* b) const
    {
        return a->key < b->key;
    }
};

static void GenerateValues(std::vector<TestNode*>* out, int n);

static void GenerateIdenticalValues(std::vector<TestNode*>* out, int n);

static void DuplicateValues(std::vector<TestNode*>* out);

template <typename T, typename V>
static void DoTest(T* checker, const std::vector<V*>& values);

template <typename T>
static void ConstTest();

typedef IntrusiveRBTree<
    int,
    TestNode,
    &TestNode::key,
    &TestNode::node> RBTreeTestType;

template <typename TreeType, typename CheckerType>
class BaseChecker
{
public:
    typedef typename TreeType::iterator iterator;
    typedef typename TreeType::const_iterator const_iterator;

    BaseChecker()
        : mConstTree(mTree)
    {
    }

    template <typename IterType, typename CheckerIterType>
    IterType iter_check(IterType treeIter, CheckerIterType checkerIter) const
    {
        if (treeIter == mTree.end())
        {
            EXPECT_EQ(checkerIter, mChecker.end());
        }
        else
        {
            EXPECT_EQ(&(*treeIter), checkerIter->second);
        }
        return treeIter;
    }

    int count(int key) const
    {
        int ret = mChecker.count(key);
        EXPECT_EQ(ret, mTree.count(key));
        return ret;
    }

    void value_check(TestNode* v)
    {
        EXPECT_EQ(find(v->key)->key, v->key);
        lower_bound(v->key);
        upper_bound(v->key);
        count(v->key);
    }

    int erase(int key)
    {
        int size = mTree.size();
        int ret = mChecker.erase(key);
        EXPECT_EQ(ret, mTree.count(key));
        EXPECT_EQ(ret, mTree.erase(key));
        EXPECT_EQ(0, mTree.count(key));
        EXPECT_EQ(size - ret, mTree.size());
        erase_check(key);
        verify();
        return ret;
    }

    iterator erase(iterator iter)
    {
        int key = iter->key;
        int size = mTree.size();
        int count = mTree.count(key);
        typename CheckerType::iterator checkerIter = mChecker.find(key);
        for (iterator tmp(mTree.find(key)); tmp != iter; ++tmp)
        {
            ++checkerIter;
        }
        typename CheckerType::iterator checkerNext = checkerIter;
        ++checkerNext;
        mChecker.erase(checkerIter);
        iter = mTree.erase(iter);
        EXPECT_EQ(mChecker.size(), mTree.size());
        EXPECT_EQ(size - 1, mTree.size());
        EXPECT_EQ(count - 1, mTree.count(key));
        if (count == 1)
        {
            erase_check(key);
        }
        verify();
        return iter_check(iter, checkerNext);
    }

    void erase(TestNode* v)
    {
        int key = v->key;
        int size = mTree.size();
        int count = mTree.count(key);
        typename CheckerType::iterator checkerIter = mChecker.find(key);
        for (; checkerIter != mChecker.end(); ++checkerIter)
        {
            if (checkerIter->second == v)
            {
                break;
            }
        }
        mChecker.erase(checkerIter);
        mTree.erase(v);
        EXPECT_EQ(mChecker.size(), mTree.size());
        EXPECT_EQ(size - 1, mTree.size());
        EXPECT_EQ(count - 1, mTree.count(key));
        if (count == 1)
        {
            erase_check(key);
        }
    }

    void erase_check(int key)
    {
        EXPECT_TRUE(mTree.find(key) == mConstTree.end());
        EXPECT_TRUE(mConstTree.find(key) == mTree.end());
    }

    iterator begin() { return mTree.begin(); }

    const_iterator begin() const { return mTree.begin(); }

    iterator end() { return mTree.end(); }

    const_iterator end() const { return mTree.end(); }

    iterator find(int key)
    {
        return iter_check(mTree.find(key), mChecker.find(key));
    }

    const_iterator find(int key) const
    {
        return iter_check(mTree.find(key), mChecker.find(key));
    }

    iterator lower_bound(int key)
    {
        return iter_check(mTree.lower_bound(key), mChecker.lower_bound(key));
    }

    const_iterator lower_bound(int key) const
    {
        return iter_check(mTree.lower_bound(key), mChecker.lower_bound(key));
    }

    iterator upper_bound(int key)
    {
        return iter_check(mTree.upper_bound(key), mChecker.upper_bound(key));
    }

    const_iterator upper_bound(int key) const
    {
        return iter_check(mTree.upper_bound(key), mChecker.upper_bound(key));
    }

    int size() const { return mTree.size(); }

    bool empty() const { return mTree.empty(); }

    void verify() const
    {
        mTree.validate_map();
        EXPECT_EQ(mTree.size(), mChecker.size());

        typename CheckerType::const_iterator checkerIter = mChecker.begin();
        const_iterator treeIter = mTree.begin();
        for (; treeIter != mTree.end(); ++treeIter, ++checkerIter)
        {
            iter_check(treeIter, checkerIter);
        }

        for (int i = mTree.size() - 1; i >= 0; --i)
        {
            iter_check(treeIter, checkerIter);
            --treeIter;
            --checkerIter;
        }
        EXPECT_EQ(mTree.begin(), treeIter);
        EXPECT_EQ(mChecker.begin(), checkerIter);
    }

protected:
    TreeType         mTree;
    const TreeType&  mConstTree;
    CheckerType      mChecker;
};

template <typename TreeType, typename CheckerType>
class UniqueChecker : public BaseChecker<TreeType, CheckerType>
{
    typedef BaseChecker<TreeType, CheckerType> super_type;

public:
    typedef typename super_type::iterator iterator;

    UniqueChecker()
        : super_type()
    {
    }

    std::pair<iterator, bool> insert(TestNode* v)
    {
        int size = this->mTree.size();
        std::pair<typename CheckerType::iterator, bool> checkerRes =
            this->mChecker.insert(std::make_pair(v->key, v));
        std::pair<iterator, bool> treeRes = this->mTree.insert(v);
        EXPECT_EQ(checkerRes.first->second, &(*treeRes.first));
        EXPECT_EQ(this->mChecker.size(), this->mTree.size());
        EXPECT_EQ(this->mTree.size(), size + treeRes.second);
        super_type::verify();
        return treeRes;
    }

    iterator insert(iterator hint, TestNode* v)
    {
        int size = this->mTree.size();
        std::pair<typename CheckerType::iterator, bool> checkerRes =
            this->mChecker.insert(std::make_pair(v->key, v));
        iterator treeRes = this->mTree.insert(hint, v);
        EXPECT_EQ(checkerRes.first->second, &(*treeRes));
        EXPECT_EQ(this->mChecker.size(), this->mTree.size());
        EXPECT_EQ(this->mTree.size(), size + checkerRes.second);
        super_type::verify();
        return treeRes;
    }
};

template <typename TreeType, typename CheckerType>
class MultiChecker : public BaseChecker<TreeType, CheckerType>
{
    typedef BaseChecker<TreeType, CheckerType> super_type;

public:
    typedef typename super_type::iterator iterator;

    MultiChecker()
        : super_type()
    {
    }

    iterator insert(TestNode* v)
    {
        int size = this->mTree.size();
        typename CheckerType::iterator checkerRes =
            this->mChecker.insert(std::make_pair(v->key, v));
        iterator treeRes = this->mTree.insert(v);
        EXPECT_EQ(checkerRes->second, &(*treeRes));
        EXPECT_EQ(this->mChecker.size(), this->mTree.size());
        EXPECT_EQ(this->mTree.size(), size + 1);
        super_type::verify();
        return treeRes;
    }

    iterator insert(iterator hint, TestNode* v)
    {
        int size = this->mTree.size();
        typename CheckerType::iterator checkerRes =
            this->mChecker.insert(std::make_pair(v->key, v));
        iterator treeRes = this->mTree.insert(hint, v);
        EXPECT_EQ(checkerRes->second, &(*treeRes));
        EXPECT_EQ(this->mChecker.size(), this->mTree.size());
        EXPECT_EQ(this->mTree.size(), size + 1);
        super_type::verify();
        return treeRes;
    }
};

TEST(IntrusiveRBTree, SimpleUnique)
{
    RBTreeTestType t;
    t.validate_tree();
    EXPECT_TRUE(t.empty());
    EXPECT_EQ(0, t.size());
    EXPECT_EQ(t.begin(), t.end());
    TestNode n;
    n.key = 1;
    n.value1 = 2;
    n.value2 = 3;
    std::pair<RBTreeTestType::iterator, bool> ret = t.insert_unique(&n);
    EXPECT_TRUE(ret.second);
    EXPECT_EQ(ret.first->key, n.key);
    EXPECT_EQ(ret.first->key, t.begin()->key);
    EXPECT_FALSE(t.empty());
    EXPECT_EQ(1, t.size());
    EXPECT_EQ(1, t.count(n.key));

    TestNode n2;
    n2.key = 1;
    ret = t.insert_unique(&n2);
    EXPECT_FALSE(ret.second);
    EXPECT_EQ(&n, &(*ret.first));

    EXPECT_EQ(&n, &(*t.lower_bound(1)));
    EXPECT_EQ(t.end(), t.upper_bound(1));
    t.validate_tree();

    t.erase(&n);
    EXPECT_TRUE(t.empty());
    EXPECT_EQ(0, t.size());
    EXPECT_EQ(t.begin(), t.end());
    t.validate_tree();
}

TEST(IntrusiveRBTree, SimpleMulti)
{
    RBTreeTestType t;
    TestNode n1;
    n1.key = 1;
    TestNode n2;
    n2.key = 1;
    TestNode n3;
    n3.key = 1;
    RBTreeTestType::iterator ret = t.insert_multi(&n1);
    EXPECT_EQ(&n1, &(*ret));
    ret = t.insert_multi(&n2);
    EXPECT_EQ(&n2, &(*ret));
    ret = t.insert_multi(&n3);
    EXPECT_EQ(&n3, &(*ret));
    ret++;
    EXPECT_EQ(t.end(), ret);
    EXPECT_EQ(3, t.count(1));
    EXPECT_EQ(3, t.size());
    t.validate_tree();

    t.erase(&n2);
    EXPECT_EQ(2, t.size());
    EXPECT_EQ(2, t.count(1));
    t.validate_tree();

    EXPECT_EQ(0, t.erase(0));
    EXPECT_EQ(2, t.erase(1));
    EXPECT_EQ(0, t.size());
}

TEST(IntrusiveRBTree, UniqueRandom)
{
    typedef IntrusiveMap<
        int,
        TestNode,
        &TestNode::key,
        &TestNode::node> TreeType;
    typedef std::map<int, TestNode*> CheckerType;
    ConstTest<UniqueChecker<TreeType, CheckerType> >();

    UniqueChecker<TreeType, CheckerType> checker;
    std::vector<TestNode*> randomValues;
    GenerateValues(&randomValues, 500);
    std::vector<TestNode*> sortedValues(randomValues);

    std::sort(sortedValues.begin(), sortedValues.end(), TestNodeLess());
    DoTest(&checker, sortedValues);

    std::reverse(sortedValues.begin(), sortedValues.end());
    DoTest(&checker, sortedValues);

    DoTest(&checker, randomValues);

    FOREACH(iter, randomValues)
    {
        delete *iter;
    }
}

TEST(IntrusiveRBTree, MultiRandom)
{
    typedef IntrusiveMultiMap<
        int,
        TestNode,
        &TestNode::key,
        &TestNode::node> TreeType;
    typedef std::multimap<int, TestNode*> CheckerType;
    ConstTest<MultiChecker<TreeType, CheckerType> >();

    MultiChecker<TreeType, CheckerType> checker;
    std::vector<TestNode*> randomValues;
    GenerateValues(&randomValues, 500);
    std::vector<TestNode*> sortedValues(randomValues);

    std::sort(sortedValues.begin(), sortedValues.end(), TestNodeLess());
    DoTest(&checker, sortedValues);

    std::reverse(sortedValues.begin(), sortedValues.end());
    DoTest(&checker, sortedValues);

    DoTest(&checker, randomValues);

    DuplicateValues(&randomValues);
    DoTest(&checker, randomValues);

    std::vector<TestNode*> identicalValues;
    GenerateIdenticalValues(&identicalValues, 100);
    DoTest(&checker, identicalValues);

    FOREACH(iter, randomValues)
    {
        delete *iter;
    }
    FOREACH(iter, identicalValues)
    {
        delete *iter;
    }
}

static void GenerateValues(std::vector<TestNode*>* out, int n)
{
    std::set<int> keys;
    int maxKey = n * 4;
    for (int i = 0; i < n; ++i)
    {
        int key = 0;
        do
        {
            key = rand() % (maxKey + 1);  // NOLINT(runtime/threadsafe_fn)
        } while (keys.find(key) != keys.end());
        TestNode* node = new TestNode;
        node->key = key;
        node->value1 = key + 1;
        node->value2 = key + 2;
        out->push_back(node);
        keys.insert(key);
    }
}

template <typename T, typename V>
static void DoTest(T* checker, const std::vector<V*>& values)
{
    T& mutableChecker = *checker;
    const T& constChecker = *checker;

    // Test insert
    FOREACH(iter, values)
    {
        mutableChecker.insert(*iter);
        mutableChecker.value_check(*iter);
    }
    ASSERT(mutableChecker.size() == values.size());
    constChecker.verify();

    // Test erase via key
    FOREACH(iter, values)
    {
        mutableChecker.erase((*iter)->key);
        EXPECT_EQ(mutableChecker.erase((*iter)->key), 0);
    }
    constChecker.verify();
    EXPECT_EQ(0, constChecker.size());

    // Test insert with hint upper bound
    FOREACH(iter, values)
    {
        mutableChecker.insert(mutableChecker.upper_bound((*iter)->key), *iter);
        mutableChecker.value_check(*iter);
    }
    ASSERT(mutableChecker.size() == values.size());
    constChecker.verify();

    // Test erase via iterator
    FOREACH(iter, values)
    {
        mutableChecker.erase(mutableChecker.find((*iter)->key));
    }
    constChecker.verify();
    EXPECT_EQ(0, constChecker.size());

    // Test insert with hint lower bound
    FOREACH(iter, values)
    {
        mutableChecker.insert(mutableChecker.lower_bound((*iter)->key), *iter);
        mutableChecker.value_check(*iter);
    }
    ASSERT(mutableChecker.size() == values.size());
    constChecker.verify();

    // Test erase via value
    FOREACH(iter, values)
    {
        mutableChecker.erase(*iter);
    }

    // Test insert with hint random
    FOREACH(iter, values)
    {
        mutableChecker.insert(mutableChecker.find(
                    values[rand() % values.size()]->key), *iter); // NOLINT(runtime/threadsafe_fn)
        mutableChecker.value_check(*iter);
    }
    ASSERT(mutableChecker.size() == values.size());
    constChecker.verify();

    // Test erase via value
    FOREACH(iter, values)
    {
        mutableChecker.erase(*iter);
    }
    constChecker.verify();
    EXPECT_EQ(0, constChecker.size());
}

template <typename T>
static void ConstTest()
{
    T mutableChecker;
    const T& constChecker = mutableChecker;
    TestNode n;
    n.key = 1;
    mutableChecker.insert(&n);
    EXPECT_NE(mutableChecker.find(n.key), constChecker.end());
    EXPECT_NE(constChecker.find(n.key), mutableChecker.end());
    EXPECT_EQ(&n, &(*constChecker.lower_bound(n.key)));
    EXPECT_EQ(constChecker.end(), constChecker.upper_bound(n.key));

    typename T::iterator mutableIter = mutableChecker.begin();
    EXPECT_EQ(mutableIter, constChecker.begin());
    EXPECT_NE(mutableIter, constChecker.end());
    EXPECT_EQ(constChecker.begin(), mutableIter);
    EXPECT_NE(constChecker.end(), mutableIter);

    typename T::const_iterator constIter = mutableIter;
    EXPECT_EQ(constIter, mutableChecker.begin());
    EXPECT_NE(constIter, mutableChecker.end());
    EXPECT_EQ(mutableChecker.begin(), constIter);
    EXPECT_NE(mutableChecker.end(), constIter);

    constChecker.verify();
    EXPECT_FALSE(constChecker.empty());
    EXPECT_EQ(1, constChecker.size());
    EXPECT_EQ(1, constChecker.count(n.key));

    mutableChecker.erase(&n);
}

static void DuplicateValues(std::vector<TestNode*>* out)
{
    size_t size = out->size();
    for (size_t i = 0; i < size; ++i)
    {
        TestNode* newNode = new TestNode;
        newNode->key = (*out)[i]->key;
        newNode->value1 = (*out)[i]->value1;
        newNode->value2 = (*out)[i]->value2;
        out->push_back(newNode);
    }
}

static void GenerateIdenticalValues(std::vector<TestNode*>* out, int n)
{
    for (int i = 0; i < n; ++i)
    {
        TestNode* node = new TestNode;
        node->key = 2;
        node->value1 = 2;
        node->value2 = 2;
        out->push_back(node);
    }
}
