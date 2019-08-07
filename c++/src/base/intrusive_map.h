#ifndef _SRC_BASE_INTRUSIVE_MAP_H
#define _SRC_BASE_INTRUSIVE_MAP_H

#include <functional>
#include <utility>

#include "src/base/intrusive_rbtree.h"
#include "src/common/macro.h"

typedef IntrusiveRBTreeNode MapLinkNode;

/**
 * Usage:
 *
 *     struct MyNode
 *     {
 *         int key;
 *         int value;
 *         MapLinkNode node;
 *     };
 *
 *     IntrusiveMap<int, MyNode, &MyNode::key, &MyNode::node> map;
 *     NyNode a;
 *     map.insert(&a);
 *     map.erase(&a);
 */
template<
    typename Key,
    typename Value,
    Key Value::*KeyMember,
    MapLinkNode Value::*LinkMember,
    typename Comparator = std::less<Key> >
class IntrusiveMap
{
public:
    typedef Key key_type;
    typedef Value value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;

    typedef IntrusiveRBTree<Key, Value, KeyMember, LinkMember, Comparator> Tree;
    typedef typename Tree::iterator iterator;
    typedef typename Tree::const_iterator const_iterator;

    explicit IntrusiveMap(const Comparator& comparator = Comparator())
        : mTree(comparator)
    {
    }

    ~IntrusiveMap() {}

    // Iterator routines
    iterator begin() { return mTree.begin(); }
    const_iterator begin() const { return mTree.begin(); }
    iterator end() { return mTree.end(); }
    const_iterator end() const { return mTree.end(); }

    // Lookup routines
    iterator find(const key_type& key) { return mTree.find(key); }
    const_iterator find(const key_type& key) const { return mTree.find(key); }
    iterator lower_bound(const key_type& key) { return mTree.lower_bound(key); }
    const_iterator lower_bound(const key_type& key) const { return mTree.lower_bound(key); }
    iterator upper_bound(const key_type& key) { return mTree.upper_bound(key); }
    const_iterator upper_bound(const key_type& key) const { return mTree.upper_bound(key); }
    size_type count(const key_type& key) const
    {
        size_type ret = mTree.count(key);
        ASSERT_DEBUG(ret <= 1);
        return ret;
    }

    // Insertion routines
    std::pair<iterator, bool> insert(pointer v) { return mTree.insert_unique(v); }
    iterator insert(iterator hint, pointer v) { return mTree.insert_unique(hint, v); }

    // Deletion routines
    size_type erase(const key_type& key)
    {
        int ret = mTree.erase(key);
        ASSERT_DEBUG(ret <= 1);
        return ret;
    }
    iterator erase(const iterator& iter) { return mTree.erase(iter); }
    void erase(pointer v) { mTree.erase(v); }

    // Size routines
    size_type size() const { return mTree.size(); }
    bool empty() const { return mTree.empty(); }

    // Utility routines
    void clear() { mTree.clear(); }
    void validate_map() const { mTree.validate_tree(); }

private:
    Tree  mTree;

    DISALLOW_COPY_AND_ASSIGN(IntrusiveMap);
};

template<
    typename Key,
    typename Value,
    Key Value::*KeyMember,
    MapLinkNode Value::*LinkMember,
    typename Comparator = std::less<Key> >
class IntrusiveMultiMap
{
public:
    typedef Key key_type;
    typedef Value value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;

    typedef IntrusiveRBTree<Key, Value, KeyMember, LinkMember, Comparator> Tree;
    typedef typename Tree::iterator iterator;
    typedef typename Tree::const_iterator const_iterator;

    explicit IntrusiveMultiMap(const Comparator& comparator = Comparator())
        : mTree(comparator)
    {
    }

    ~IntrusiveMultiMap() {}

    // Iterator routines
    iterator begin() { return mTree.begin(); }
    const_iterator begin() const { return mTree.begin(); }
    iterator end() { return mTree.end(); }
    const_iterator end() const { return mTree.end(); }

    // Lookup routines
    iterator find(const key_type& key) { return mTree.find(key); }
    const_iterator find(const key_type& key) const { return mTree.find(key); }
    iterator lower_bound(const key_type& key) { return mTree.lower_bound(key); }
    const_iterator lower_bound(const key_type& key) const { return mTree.lower_bound(key); }
    iterator upper_bound(const key_type& key) { return mTree.upper_bound(key); }
    const_iterator upper_bound(const key_type& key) const { return mTree.upper_bound(key); }
    size_type count(const key_type& key) const { return mTree.count(key); }

    // Insertion routines
    iterator insert(pointer v) { return mTree.insert_multi(v); }
    iterator insert(iterator hint, pointer v) { return mTree.insert_multi(hint, v); }

    // Deletion routines
    size_type erase(const key_type& key) { return mTree.erase(key); }
    iterator erase(const iterator& iter) { return mTree.erase(iter); }
    void erase(pointer v) { mTree.erase(v); }

    // Size routines
    size_type size() const { return mTree.size(); }
    bool empty() const { return mTree.empty(); }

    // Utility routines
    void clear() { mTree.clear(); }
    void validate_map() const { mTree.validate_tree(); }

private:
    Tree  mTree;

    DISALLOW_COPY_AND_ASSIGN(IntrusiveMultiMap);
};

#endif // _SRC_BASE_INTRUSIVE_MAP_H
