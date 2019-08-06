#ifndef _SRC_BASE_INTRUSIVE_RBTREE_H
#define _SRC_BASE_INTRUSIVE_RBTREE_H

#include <stddef.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <utility>

#include "src/common/macro.h"
#include "src/common/assert.h"
#include "src/common/common.h"

struct IntrusiveRBTreeNode
{
    IntrusiveRBTreeNode* parent;
    IntrusiveRBTreeNode* left;
    IntrusiveRBTreeNode* right;
    bool isRed; // Is a red node.
    bool isL; // Is left child of parent.  Always true for root.

    IntrusiveRBTreeNode()
        : parent(NULL),
          left(NULL),
          right(NULL),
          isRed(false),
          isL(false)
    {
    }

    IntrusiveRBTreeNode** child(bool isL)
    {
        return isL ? &left : &right;
    }
};

template<
    typename Key,
    typename Value,
    Key Value::*KeyMember,
    IntrusiveRBTreeNode Value::*LinkMember,
    typename Comparator = std::less<Key> >
class IntrusiveRBTree
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

    class iterator;
    class const_iterator;

    explicit IntrusiveRBTree(const Comparator& comparator = Comparator())
        : mRoot(NULL),
          mNull(NULL),
          mSize(0),
          mComparator(comparator)
    {
    }

    size_type size() const
    {
        return mSize;
    }

    iterator begin()
    {
        return iterator(this, _next(NULL,  /* isBackward */ false));
    }

    const_iterator begin() const
    {
        return const_cast<IntrusiveRBTree*>(this)->begin();
    }

    iterator end()
    {
        return iterator(this, NULL);
    }

    const_iterator end() const
    {
        return const_cast<IntrusiveRBTree*>(this)->end();
    }

    iterator find(const Key& key)
    {
        iterator iter = lower_bound(key);
        if (iter.get_node() != NULL &&
                !mComparator(key, (*iter).*KeyMember))
        {
            return iter;
        }
        return iterator(this, NULL);
    }

    const_iterator find(const Key& key) const
    {
        return const_cast<IntrusiveRBTree*>(this)->find(key);
    }

    iterator lower_bound(const Key& key)
    {
        IntrusiveRBTreeNode* x = mRoot;
        IntrusiveRBTreeNode* y = NULL;
        while (x != NULL)
        {
            Value* v = _get_value_ptr(x);
            if (mComparator(v->*KeyMember, key))
            {
                x = x->right;
            }
            else
            {
                y = x;
                x = x->left;
            }
        }
        return iterator(this, y);
    }

    const_iterator lower_bound(const Key& key) const
    {
        return const_cast<IntrusiveRBTree*>(this)->lower_bound(key);
    }

    iterator upper_bound(const Key& key)
    {
        IntrusiveRBTreeNode* x = mRoot;
        IntrusiveRBTreeNode* y = NULL;
        while (x != NULL)
        {
            Value* v = _get_value_ptr(x);
            if (!mComparator(key, v->*KeyMember))
            {
                x = x->right;
            }
            else
            {
                y = x;
                x = x->left;
            }
        }
        return iterator(this, y);
    }

    const_iterator upper_bound(const Key& key) const
    {
        return const_cast<IntrusiveRBTree*>(this)->upper_bound(key);
    }

    bool empty() const
    {
        return mRoot == NULL;
    }

    size_type count(const Key& key) const
    {
        const_iterator iter = lower_bound(key);
        size_type ret = 0;
        while (iter.get_node() != NULL && !mComparator(key, (*iter).*KeyMember))
        {
            ret++;
            iter++;
        }
        return ret;
    }

    std::pair<iterator, bool> insert_unique(Value* v)
    {
        iterator iter = lower_bound(v->*KeyMember);
        IntrusiveRBTreeNode* parent = iter.get_node();
        if (parent != NULL && !mComparator(v->*KeyMember, (*iter).*KeyMember))
        {
            return std::pair<iterator, bool>(iter, false);
        }
        _insert_internal(parent, &(v->*LinkMember), /* isL */ true);
        return std::pair<iterator, bool>(iterator(this, &(v->*LinkMember)), true);
    }

    iterator insert_unique(iterator hint, Value* v)
    {
        return _insert_with_hint(hint, v, /* isMulti */ false);
    }

    iterator insert_multi(Value* v)
    {
        iterator iter = upper_bound(v->*KeyMember);
        _insert_internal(iter.get_node(), &(v->*LinkMember), /* isL */ true);
        return iterator(this, &(v->*LinkMember));
    }

    iterator insert_multi(iterator hint, Value* v)
    {
        return _insert_with_hint(hint, v, /* isMulti */ true);
    }

    size_type erase(const Key& key)
    {
        iterator iter = lower_bound(key);
        size_type count = 0;
        while (iter.get_node() != NULL &&
                !mComparator(key, (*iter).*KeyMember))
        {
            iter = erase(iter);
            count++;
        }
        return count;
    }

    void erase(Value* v)
    {
        iterator iter(this, &(v->*LinkMember));
        erase(iter);
    }

    iterator erase(iterator iter)
    {
        IntrusiveRBTreeNode* ref = iter.get_node();
        Value* node = _get_value_ptr(ref);
        iterator nextIter = iter;
        nextIter++;
        IntrusiveRBTreeNode** nodeRef = (node->*LinkMember).parent == NULL ?
            &mRoot : (node->*LinkMember).parent->child(ref->isL);
        bool needAdjust = false;
        bool isL = false;
        IntrusiveRBTreeNode** parentRef = NULL;
        Value* parent = NULL;
        if ((node->*LinkMember).left == NULL)
        {
            parentRef = _get_parent_node_ref(*nodeRef);
            parent = _get_value_ptr(*parentRef);
            isL = (node->*LinkMember).isL;
            needAdjust = !(node->*LinkMember).isRed;
            _remove(nodeRef, false);
            node = _get_value_ptr(*nodeRef);
        }
        else if ((node->*LinkMember).right == NULL)
        {
            parentRef = _get_parent_node_ref(*nodeRef);
            parent = _get_value_ptr(*parentRef);
            isL = (node->*LinkMember).isL;
            needAdjust = !(node->*LinkMember).isRed;
            _remove(nodeRef, true);
            node = _get_value_ptr(*nodeRef);
        }
        else
        {
            IntrusiveRBTreeNode** childRef = &(node->*LinkMember).left;
            Value* child = _get_value_ptr(*childRef);
            while ((child->*LinkMember).right != NULL)
            {
                childRef = &(child->*LinkMember).right;
                child = _get_value_ptr(*childRef);
            }
            parentRef = _get_parent_node_ref(*childRef);
            parent = _get_value_ptr(*parentRef);
            isL = (child->*LinkMember).isL;
            needAdjust = !(child->*LinkMember).isRed;
            IntrusiveRBTreeNode* childRefBackup = *childRef;
            Value* childBackup = child;
            _remove(childRef, true);
            childBackup->*LinkMember = node->*LinkMember;
            *nodeRef = childRefBackup;
            if ((node->*LinkMember).left != NULL)
            {
                (node->*LinkMember).left->parent = childRefBackup;
            }
            if ((node->*LinkMember).right != NULL)
            {
                (node->*LinkMember).right->parent = childRefBackup;
            }
            if (isL)
            {
                parent = childBackup;
                nodeRef = &(childBackup->*LinkMember).left;
            }
            else
            {
                nodeRef = childRef;
                if (parentRef == &(node->*LinkMember).left)
                {
                    parentRef = &(childBackup->*LinkMember).left;
                }
            }
            node = child;
        }
        ASSERT_DEBUG(mSize > 0);
        --mSize;

        while (needAdjust)
        {
            if (*parentRef == NULL)
            {
                if (*nodeRef != NULL)
                {
                    (node->*LinkMember).isRed = false;
                }
                break;
            }
            IntrusiveRBTreeNode** siblingRef = (parent->*LinkMember).child(!isL);
            Value* sibling = _get_value_ptr(*siblingRef);
            if ((sibling->*LinkMember).isRed)
            {
                _rotate(parentRef, &parent, siblingRef, &sibling);
                (parent->*LinkMember).isRed = false;
                (sibling->*LinkMember).isRed = true;
                parentRef = (parent->*LinkMember).child(isL);
                parent = sibling;
                siblingRef = (parent->*LinkMember).child(!isL);
                sibling = _get_value_ptr(*siblingRef);
            }
            IntrusiveRBTreeNode** scRef;
            Value* sc;
            if (*(scRef = (sibling->*LinkMember).child(isL)) != NULL &&
                ((sc = _get_value_ptr(*scRef))->*LinkMember).isRed)
            {
                _rotate(siblingRef, &sibling, scRef, &sc);
                (sibling->*LinkMember).isRed = false;
                (sc->*LinkMember).isRed = true;
            }
            if (*(scRef = (sibling->*LinkMember).child(!isL)) != NULL &&
                ((sc = _get_value_ptr(*scRef))->*LinkMember).isRed)
            {
                _rotate(parentRef, &parent, siblingRef, &sibling);
                (parent->*LinkMember).isRed = (sibling->*LinkMember).isRed;
                (sibling->*LinkMember).isRed = false;
                (sc->*LinkMember).isRed = false;
                needAdjust = false;
            }
            else
            {
                needAdjust = !(parent->*LinkMember).isRed;
                (parent->*LinkMember).isRed = false;
                (sibling->*LinkMember).isRed = true;
                if (needAdjust)
                {
                    nodeRef = parentRef;
                    node = parent;
                    isL = (node->*LinkMember).isL;
                    parentRef = _get_parent_node_ref(*nodeRef);
                    parent = _get_value_ptr(*parentRef);
                }
            }
        }
        return nextIter;
    }

    void clear()
    {
        mRoot = NULL;
        mSize = 0;
    }

    void validate_tree() const
    {
        size_type size = 0;
        int expectLevel = -1;
        size = validate(mRoot, NULL, true, 0, &expectLevel, true);
        ASSERT(size == mSize);
    }

    size_type validate(IntrusiveRBTreeNode* node,
                       IntrusiveRBTreeNode* parent,
                       bool isL,
                       int level,
                       int* expectLevel,
                       bool expectBlack) const
    {
        if (node == NULL)
        {
            if (*expectLevel == -1)
            {
                *expectLevel = level;
            }
            else
            {
                ASSERT(*expectLevel == level);
            }
            return 0;
        }
        size_type size = 1;
        ASSERT(node->parent == parent);
        ASSERT(node->isL == isL);
        if (!node->isRed)
        {
            ++level;
        }
        else
        {
            ASSERT(!expectBlack);
        }
        size += validate(node->left, node, true, level, expectLevel, node->isRed);
        size += validate(node->right, node, false, level, expectLevel, node->isRed);
        return size;
    }

    void show_tree() const
    {
        show(mRoot, true, "");
    }

    void show(IntrusiveRBTreeNode* node, bool isL, const std::string& prefix) const
    {
        if (node != NULL)
        {
            show(node->left, true,
                 prefix + (node == mRoot || isL ? "    " : "|   "));
        }
        std::cout << prefix;
        std::cout << (node == mRoot ? "-" : isL ? "/" : "\\");
        if (node != NULL)
        {
            std::cout << (node->isRed ? "[R]" : "[B]");
            std::cout << _get_value_ptr(node)->*KeyMember;
        }
        else
        {
            std::cout << "[NULL]";
        }
        std::cout << std::endl;
        if (node != NULL)
        {
            show(node->right, false,
                 prefix + (node == mRoot || !isL ? "    " : "|   "));
        }
    }

private:
    void _rotate(IntrusiveRBTreeNode** nodeRef, Value** node,
                 IntrusiveRBTreeNode** childRef, Value** child)
    {
        ASSERT_DEBUG(_get_value_ptr(*childRef) == *child);
        ASSERT_DEBUG(_get_value_ptr(*nodeRef) == *node);
        bool isL = (*childRef)->isL;
        IntrusiveRBTreeNode** childRef2 = (*childRef)->child(!isL);
        if (*childRef2 != NULL)
        {
            (*childRef2)->parent = *nodeRef;
            (*childRef2)->isL = isL;
        }
        (*childRef)->parent = (*nodeRef)->parent;
        (*childRef)->isL = (*nodeRef)->isL;
        IntrusiveRBTreeNode* newRoot = (*nodeRef)->parent = *childRef;
        ((*node)->*LinkMember).isL = !isL;
        *childRef = *childRef2;
        *childRef2 = *nodeRef;
        *nodeRef = newRoot;
        std::swap(*node, *child);
    }

    void _remove(IntrusiveRBTreeNode** node, bool isL)
    {
        IntrusiveRBTreeNode** child = (*node)->child(isL);
        if (*child != NULL)
        {
            (*child)->parent = (*node)->parent;
            (*child)->isL = (*node)->isL;
        }
        *node = *child;
    }

    IntrusiveRBTreeNode* _next(IntrusiveRBTreeNode* node, bool isBackward) const
    {
        // The next operation will iterate the node reference to the next one.
        // isBackward decides the order of iteration.  Use true for backward iterate
        // and false for forward iterate.  The NULL reference stands for the
        // end node in both forward and backward.  Here, we let the next node of
        // end node be the begin node, so that this method can be applied to any
        // valid node (including end node) in this map without any error.
        IntrusiveRBTreeNode* node2 = NULL;
        if (UNLIKELY(node == NULL))
        {
            if (mRoot == NULL)
            {
                return NULL;
            }
            node = mRoot;
            while ((node2 = *node->child(!isBackward)) != NULL)
            {
                node = node2;
            }
            return node;
        }
        else
        {
            node2 = *node->child(isBackward);
            if (node2 != NULL)
            {
                node = node2;
                while ((node2 = *node->child(!isBackward)) != NULL)
                {
                    node = node2;
                }
                return node;
            }
            else
            {
                while ((node2 = node->parent) != NULL && node->isL == isBackward)
                {
                    node = node2;
                }
                return node2;
            }
        }
    }

    void _insert_node(IntrusiveRBTreeNode** parentRef,
                      IntrusiveRBTreeNode** nodeRef,
                      IntrusiveRBTreeNode* newNode)
    {
        Value* node = _get_value_ptr(newNode);
        *nodeRef = newNode;
        Value* parent = _get_value_ptr(*parentRef);
        (*nodeRef)->parent = *parentRef;
        (*nodeRef)->left = NULL;
        (*nodeRef)->right = NULL;
        (*nodeRef)->isRed = true;
        for (;;)
        {
            if (*parentRef == NULL)
            {
                (node->*LinkMember).isRed = false;
                break;
            }
            else if (!(parent->*LinkMember).isRed)
            {
                break;
            }
            else
            {
                IntrusiveRBTreeNode** parentRef2 = _get_parent_node_ref(*parentRef);
                Value* parent2 = _get_value_ptr(*parentRef2);
                if ((node->*LinkMember).isL != (parent->*LinkMember).isL)
                {
                    _rotate(parentRef, &parent, nodeRef, &node);
                }
                _rotate(parentRef2, &parent2, parentRef, &parent);
                (node->*LinkMember).isRed = false;
                nodeRef = parentRef2;
                node = parent2;
                parentRef = _get_parent_node_ref(*nodeRef);
                parent = _get_value_ptr(*parentRef);
            }
        }
        ++mSize;
    }

    IntrusiveRBTreeNode** _get_parent_node_ref(IntrusiveRBTreeNode* node)
    {
        IntrusiveRBTreeNode* parent = node->parent;
        if (parent == NULL)
        {
            return &mNull;
        }
        if (parent == mRoot)
        {
            return &mRoot;
        }
        return parent->parent->child(parent->isL);
    }

    void _insert_internal(IntrusiveRBTreeNode* parent, IntrusiveRBTreeNode* node, bool isL)
    {
        if (UNLIKELY(parent == NULL))
        {
            if (mRoot == NULL)
            {
                node->isL = true;
                _insert_node(&mNull, &mRoot, node);
            }
            else
            {
                node->isL = false;
                parent = _next(parent, /* isBackward */ true);
                ASSERT_DEBUG(parent != NULL);
                IntrusiveRBTreeNode** parentRef = parent->parent == NULL ?
                    &mRoot : parent->parent->child(parent->isL);
                ASSERT_DEBUG((*parentRef)->right == NULL);
                _insert_node(parentRef, &(*parentRef)->right, node);
            }
        }
        else
        {
            if (*parent->child(isL) == NULL)
            {
                node->isL = isL;
                IntrusiveRBTreeNode** parentRef = parent->parent == NULL ?
                    &mRoot : parent->parent->child(parent->isL);
                _insert_node(parentRef, parent->child(isL), node);
            }
            else
            {
                node->isL = !isL;
                parent = _next(parent, /* isBackward */ isL);
                ASSERT_DEBUG(parent != NULL);
                IntrusiveRBTreeNode** parentRef = parent->parent == NULL ?
                    &mRoot : parent->parent->child(parent->isL);
                ASSERT_DEBUG(*parent->child(!isL)== NULL);
                _insert_node(parentRef, parent->child(!isL), node);
            }
        }
    }

    void _insert_node_between(IntrusiveRBTreeNode** x,
                              IntrusiveRBTreeNode** y,
                              IntrusiveRBTreeNode* node)
    {
        if ((*x)->right == NULL)
        {
            node->isL = false;
            _insert_node(x, &(*x)->right, node);
        }
        else
        {
            ASSERT_DEBUG((*y)->left == NULL);
            node->isL = true;
            _insert_node(y, &(*y)->left, node);
        }
    }

    iterator _insert_with_hint(iterator hint, Value* v, bool isMulti)
    {
        if (UNLIKELY(hint.get_node() == NULL))
        {
            // Because we don't maintain the right-most node,
            // just use the original insert.
            return isMulti ? insert_multi(v) : insert_unique(v).first;
        }
        IntrusiveRBTreeNode** hintRef = hint.get_node()->parent == NULL ?
            &mRoot : hint.get_node()->parent->child(hint.get_node()->isL);
        Value* hintNode = _get_value_ptr(*hintRef);
        if (mComparator(v->*KeyMember, hintNode->*KeyMember))
        {
            // less than hint
            IntrusiveRBTreeNode* node = _next(*hintRef,  /* isBackward */ true);
            if (node == NULL)
            {
                (v->*LinkMember).isL = true;
                _insert_node(hintRef, &(*hintRef)->left, &(v->*LinkMember));
                return iterator(this, &(v->*LinkMember));
            }
            IntrusiveRBTreeNode** prevNodeRef = node->parent == NULL ?
                &mRoot : node->parent->child(node->isL);
            Value* prevNode = _get_value_ptr(node);
            if (mComparator(prevNode->*KeyMember, v->*KeyMember))
            {
                // The new node should insert before the hint
                _insert_node_between(prevNodeRef, hintRef, &(v->*LinkMember));
                return iterator(this, &(v->*LinkMember));
            }
            else if (mComparator(v->*KeyMember, prevNode->*KeyMember))
            {
                // less than prev node of hint
                return isMulti ? insert_multi(v) : insert_unique(v).first;
            }
            else
            {
                // Equel to the node before hint
                if (isMulti)
                {
                    // The new node should insert before the hint
                    _insert_node_between(prevNodeRef, hintRef, &(v->*LinkMember));
                    return iterator(this, &(v->*LinkMember));
                }
                else
                {
                    return iterator(this, *prevNodeRef);
                }
            }
        }
        else if (isMulti || mComparator(hintNode->*KeyMember, v->*KeyMember))
        {
            // larger than or equal to hint
            IntrusiveRBTreeNode* node = _next(*hintRef,  /* isBackward */ false);
            if (node == NULL)
            {
                (v->*LinkMember).isL = false;
                _insert_node(hintRef, &(*hintRef)->right, &(v->*LinkMember));
                return iterator(this, &(v->*LinkMember));
            }
            IntrusiveRBTreeNode** nextNodeRef = node->parent == NULL ?
                &mRoot : node->parent->child(node->isL);
            Value* nextNode = _get_value_ptr(node);
            if (mComparator(v->*KeyMember, nextNode->*KeyMember))
            {
                // The new node should insert after the hint
                _insert_node_between(hintRef, nextNodeRef, &(v->*LinkMember));
                return iterator(this, &(v->*LinkMember));
            }
            if (mComparator(nextNode->*KeyMember, v->*KeyMember))
            {
                // Larger than next node of hint
                return isMulti ? insert_multi(v) : insert_unique(v).first;
            }
            else
            {
                // Equal to next node of hint
                return isMulti ? insert_multi(v) : iterator(this, *nextNodeRef);
            }
        }
        else
        {
            // equal to hint and not in multi
            return hint;
        }
    }

    static Value* _get_value_ptr(IntrusiveRBTreeNode* node)
    {
        return MemberToObject(node, LinkMember);
    }

    IntrusiveRBTreeNode* mRoot;
    IntrusiveRBTreeNode* mNull;
    size_type            mSize;
    Comparator           mComparator;

    DISALLOW_COPY_AND_ASSIGN(IntrusiveRBTree);
};

template<
    typename Key,
    typename Value,
    Key Value::*KeyMember,
    IntrusiveRBTreeNode Value::*LinkMember,
    typename Comparator>
class IntrusiveRBTree<Key, Value, KeyMember, LinkMember, Comparator>
    ::iterator
{
public:
    typedef iterator self;
    typedef Value value_type;
    typedef ptrdiff_t difference_type;
    typedef Value* pointer;
    typedef Value& reference;
    typedef std::bidirectional_iterator_tag iterator_category;

    iterator()
        : mTree(NULL),
          mNode(NULL)
    {
    }

    iterator(const iterator& other)
        : mTree(other.mTree),
          mNode(other.mNode)
    {
    }

    iterator& operator=(const iterator& other)
    {
        mTree = other.mTree;
        mNode = other.mNode;
        return *this;
    }

    bool operator==(const iterator& other) const
    {
        return mTree == other.mTree && mNode == other.mNode;
    }

    bool operator!=(const iterator& other) const
    {
        return mTree != other.mTree || mNode != other.mNode;
    }

    bool operator==(const const_iterator& other) const
    {
        return mTree == other.mTree && mNode == other.mNode;
    }

    bool operator!=(const const_iterator& other) const
    {
        return mTree != other.mTree || mNode != other.mNode;
    }

    pointer operator->() const
    {
        return mTree->_get_value_ptr(mNode);
    }

    reference operator*() const
    {
        return *mTree->_get_value_ptr(mNode);
    }

    iterator& operator++()
    {
        mNode = mTree->_next(mNode, /* isBackward */ false);
        return *this;
    }

    iterator& operator--()
    {
        mNode = mTree->_next(mNode,  /* isBackward */ true);
        return *this;
    }

    iterator operator++(int)
    {
        iterator ret = *this;
        operator++();
        return ret;
    }

    iterator operator--(int)
    {
        iterator ret = *this;
        operator--();
        return ret;
    }

private:
    friend class IntrusiveRBTree;
    friend class const_iterator;

    iterator(IntrusiveRBTree* t, IntrusiveRBTreeNode* n)
        : mTree(t),
          mNode(n)
    {
    }

    IntrusiveRBTreeNode* get_node() const
    {
        return mNode;
    }

    IntrusiveRBTree*     mTree;
    IntrusiveRBTreeNode* mNode;
};

template<
    typename Key,
    typename Value,
    Key Value::*KeyMember,
    IntrusiveRBTreeNode Value::*LinkMember,
    typename Comparator>
class IntrusiveRBTree<Key, Value, KeyMember, LinkMember, Comparator>
    ::const_iterator
{
public:
    typedef iterator self;
    typedef Value value_type;
    typedef ptrdiff_t difference_type;
    typedef Value* pointer;
    typedef Value& reference;
    typedef std::bidirectional_iterator_tag iterator_category;

    const_iterator()
        : mTree(NULL),
          mNode(NULL)
    {
    }

    const_iterator(const const_iterator& other)
        : mTree(other.mTree),
          mNode(other.mNode)
    {
    }

    const_iterator(const iterator& other)  // NOLINT(runtime/explicit)
        : mTree(other.mTree),
          mNode(other.mNode)
    {
    }

    const_iterator& operator=(const const_iterator& other)
    {
        mTree = other.mTree;
        mNode = other.mNode;
        return *this;
    }

    bool operator==(const const_iterator& other) const
    {
        return mTree == other.mTree && mNode == other.mNode;
    }

    bool operator!=(const const_iterator& other) const
    {
        return mTree != other.mTree || mNode != other.mNode;
    }

    bool operator==(const iterator& other) const
    {
        return mTree == other.mTree && mNode == other.mNode;
    }

    bool operator!=(const iterator& other) const
    {
        return mTree != other.mTree || mNode != other.mNode;
    }

    const_pointer operator->() const
    {
        return mTree->_get_value_ptr(mNode);
    }

    const_reference operator*() const
    {
        return *mTree->_get_value_ptr(mNode);
    }

    const_iterator& operator++()
    {
        mNode = mTree->_next(mNode,  /* isBackward */ false);
        return *this;
    }

    const_iterator& operator--()
    {
        mNode = mTree->_next(mNode,  /* isBackward */ true);
        return *this;
    }

    const_iterator operator++(int)
    {
        const_iterator ret = *this;
        operator++();
        return ret;
    }

    const_iterator operator--(int)
    {
        const_iterator ret = *this;
        operator--();
        return ret;
    }

private:
    friend class IntrusiveRBTree;
    friend class iterator;

    const_iterator(const IntrusiveRBTree* t, IntrusiveRBTreeNode* n)
        : mTree(t),
          mNode(n)
    {
    }

    IntrusiveRBTreeNode* get_node() const
    {
        return mNode;
    }

    const IntrusiveRBTree* mTree;
    IntrusiveRBTreeNode*   mNode;
};

#endif  // _SRC_BASE_INTRUSIVE_RBTREE_H
