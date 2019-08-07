#include "src/base/intrusive_map.h"

#include <gtest/gtest.h>
#include <set>

struct MapTestNode
{
    int key;
    int value1;
    double value2;
    MapLinkNode link;
};

struct NodeAllocatorArray
{
    typedef MapTestNode NodeType;

    NodeType* GetRef(int index)
    {
        return mNodes + index;
    }

    NodeType* GetPtr(int index)
    {
        return mNodes + index;
    }

    NodeType* GetPtrFromRef(NodeType* ref)
    {
        return ref;
    }

private:
    NodeType mNodes[200];
};

typedef IntrusiveMap<
    int,
    MapTestNode,
    &MapTestNode::key,
    &MapTestNode::link> MapForTest;

template<typename MapType, typename NodeAllocator>
void IntrusiveMapTest(
        MapType* map, NodeAllocator* nodeAlloc, unsigned* randSeed)
{

    EXPECT_EQ(map->count(100), 0);
    EXPECT_TRUE(map->find(100) == map->end());
    EXPECT_TRUE(map->lower_bound(100) == map->end());
    EXPECT_TRUE(map->begin() == map->end());
    EXPECT_EQ(map->size(), 0U);
    EXPECT_TRUE(map->empty());

    std::set<int> keySet;

    for (int i = 0; i < 100; ++i)
    {
        typename NodeAllocator::NodeType* node = nodeAlloc->GetPtr(i);
        node->key = rand_r(randSeed) % 200;
        node->value1 = node->key * 2;
        node->value2 = node->key / 64.0;
        std::pair<typename MapType::iterator, bool> res;
        if (rand_r(randSeed) % 2 < 1)
        {
            res = map->insert(nodeAlloc->GetRef(i));
        }
        else
        {
            int type = rand_r(randSeed) % 3;
            typename MapType::iterator mapIter;
            if (type == 0)
            {
                // smaller hint
                std::set<int>::iterator iter = keySet.lower_bound(node->key);
                if (iter != keySet.begin())
                {
                    --iter;
                    typename MapType::iterator iter2 = map->find(*iter);
                    EXPECT_TRUE(iter2 != map->end());
                    mapIter = map->insert(iter2, nodeAlloc->GetRef(i));
                    res = std::make_pair(mapIter, &(*mapIter) == nodeAlloc->GetRef(i));
                }
                else
                {
                    mapIter = map->insert(map->begin(), nodeAlloc->GetRef(i));
                    res = std::make_pair(mapIter, &(*mapIter) == nodeAlloc->GetRef(i));
                }
            }
            else if (type == 1)
            {
                // larger hint
                std::set<int>::iterator iter = keySet.upper_bound(node->key);
                if (iter != keySet.end())
                {
                    typename MapType::iterator iter2 = map->find(*iter);
                    EXPECT_TRUE(iter2 != map->end());
                    mapIter = map->insert(iter2, nodeAlloc->GetRef(i));
                    res = std::make_pair(mapIter, &(*mapIter) == nodeAlloc->GetRef(i));
                }
                else
                {
                    mapIter = map->insert(map->end(), nodeAlloc->GetRef(i));
                    res = std::make_pair(mapIter, &(*mapIter) == nodeAlloc->GetRef(i));
                }
            }
            else
            {
                // random hint
                if (keySet.empty())
                {
                    mapIter = map->insert(map->end(), nodeAlloc->GetRef(i));
                    res = std::make_pair(mapIter, &(*mapIter) == nodeAlloc->GetRef(i));
                }
                else
                {
                    int count = rand() % keySet.size(); // NOLINT
                    std::set<int>::iterator iter = keySet.begin();
                    for (int j = 0; j < count; ++j)
                    {
                        ++iter;
                    }
                    typename MapType::iterator iter2 = map->find(*iter);
                    EXPECT_TRUE(iter2 != map->end());
                    mapIter = map->insert(iter2, nodeAlloc->GetRef(i));
                    res = std::make_pair(mapIter, &(*mapIter) == nodeAlloc->GetRef(i));
                }
            }
        }
        bool isNew = keySet.insert(node->key).second;
        EXPECT_EQ(isNew, res.second);
        if (isNew)
        {
            EXPECT_TRUE(&*res.first == node);
        }
        else
        {
            EXPECT_EQ(res.first->key, node->key);
        }
        EXPECT_EQ(map->size(), keySet.size());
        EXPECT_TRUE(!map->empty());
        map->validate_map();
    }
    // map->show_map();

    std::set<int>::const_iterator keyIter = keySet.begin();
    for (typename MapType::iterator iter = map->begin();
         iter != map->end(); ++iter, ++keyIter)
    {
        EXPECT_EQ(iter->key, *keyIter);
        EXPECT_EQ(iter->value1, iter->key * 2);
        EXPECT_EQ(iter->value2, iter->key / 64.0);
        typename NodeAllocator::NodeType& node = *iter;
        EXPECT_EQ(node.key, *keyIter);

        typename MapType::iterator iter2 = map->find(iter->key);
        EXPECT_TRUE(iter2 == iter);
        iter2 = map->lower_bound(iter->key);
        EXPECT_TRUE(iter2 == iter);
        EXPECT_EQ(map->count(iter->key), 1);
    }
    EXPECT_TRUE(keyIter == keySet.end());

    const MapType* cmap = map;
    keyIter = keySet.begin();
    for (typename MapType::const_iterator iter = cmap->begin();
         iter != cmap->end(); ++iter, ++keyIter)
    {
        EXPECT_EQ(iter->key, *keyIter);
        EXPECT_EQ(iter->value1, iter->key * 2);
        EXPECT_EQ(iter->value2, iter->key / 64.0);
        const typename NodeAllocator::NodeType& node = *iter;
        EXPECT_EQ(node.key, *keyIter);

        typename MapType::const_iterator iter2 = cmap->find(iter->key);
        EXPECT_TRUE(iter2 == iter);
        iter2 = cmap->lower_bound(iter->key);
        EXPECT_TRUE(iter2 == iter);
        EXPECT_EQ(cmap->count(iter->key), 1);
    }
    EXPECT_TRUE(keyIter == keySet.end());

    for (int i = -1; i < 205; ++i)
    {
        if (!keySet.count(i))
        {
            EXPECT_TRUE(map->find(i) == map->end());
            EXPECT_TRUE(cmap->find(i) == cmap->end());
            std::set<int>::const_iterator it = keySet.lower_bound(i);
            typename MapType::iterator iter = map->lower_bound(i);
            typename MapType::const_iterator citer = cmap->lower_bound(i);
            if (it == keySet.end())
            {
                EXPECT_TRUE(iter == map->end());
                EXPECT_TRUE(citer == cmap->end());
            }
            else
            {
                EXPECT_EQ(iter->key, *it);
                EXPECT_EQ(citer->key, *it);
            }
            EXPECT_EQ(map->count(i), 0);
        }
    }

    for (int i = 0; i < 150; ++i)
    {
        int key = rand_r(randSeed) % 200;
        bool exist = keySet.count(key);
        typename MapType::iterator iter = map->find(key);
        int count = map->erase(key);
        if (exist)
        {
            EXPECT_EQ(count, 1);
            typename NodeAllocator::NodeType* node =
                nodeAlloc->GetPtrFromRef(&(*iter));
            EXPECT_EQ(node->key, key);
            keySet.erase(key);
        }
        else
        {
            EXPECT_EQ(count, 0);
            EXPECT_EQ(map->end(), iter);
        }
        map->validate_map();
        EXPECT_EQ(map->count(key), 0);
        EXPECT_EQ(map->size(), keySet.size());
    }
    // map->show_map();

    map->clear();
    EXPECT_EQ(map->size(), 0U);
    EXPECT_TRUE(map->empty());
    // map->show_map();
}

TEST(IntrusiveMap, BasicTest)
{
    unsigned randSeed = time(NULL);

    MapForTest map;
    NodeAllocatorArray allocator;
    IntrusiveMapTest(&map, &allocator, &randSeed);
}
