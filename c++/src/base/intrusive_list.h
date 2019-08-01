#ifndef _SRC_BASE_INTRUSIVE_LIST_H
#define _SRC_BASE_INTRUSIVE_LIST_H

#include <stddef.h>
#include <iterator>

#include "src/base/list.h"
#include "src/common/common.h"
#include "src/common/macro.h"

class LinkNode
{
public:
    LinkNode() { list_init(&mNode); }
    void Unlink() { list_del(&mNode); }
    bool IsSingle() const { return list_empty(&mNode); }

private:
    template <typename T, LinkNode T::*M>
    friend class IntrusiveList;

    list_t mNode;

    DISALLOW_COPY_AND_ASSIGN(LinkNode);
};

/**
 * This is a STL-style wrapper of list_t.  It differs from std::list in
 * that it does not own objects in the container.  There is no memory allocation
 * at all.  The user must manage life-cycle of objects.
 *
 * Usage:
 *
 *    struct MyNode
 *    {
 *        int      value;
 *        LinkNode node;
 *    };
 *
 *    IntrusiveList<MyNode, Mynode::node> list;
 *    MyNode a;
 *    MyNode b;
 *    list.push_back(&a);
 *    list.push_back(&b);
 *    list.pop_front();  // a is unlinked
 *    list.pop_front();  // b is unlinked
 */
template <typename T, LinkNode T::*M>
class IntrusiveList
{
public:
    class iterator
    {
    public:
        typedef iterator self;
        typedef T value_type;
        typedef ptrdiff_t difference_type;
        typedef T* pointer;
        typedef T& reference;
        typedef std::bidirectional_iterator_tag iterator_category;

        iterator() : mNode(NULL) {}
        explicit iterator(list_t* node) : mNode(node) {}

        bool operator==(const self& that) const { return mNode == that.mNode; }
        bool operator!=(const self& that) const { return !(*this == that); }

        self& operator++()
        {
            mNode = mNode->next;
            return *this;
        }

        self operator++(int)
        {
            self tmp(*this);
            ++*this;
            return tmp;
        }

        self& operator--() {
            mNode = mNode->prev;
            return *this;
        }

        self operator--(int)
        {
            self tmp(*this);
            --*this;
            return tmp;
        }

        pointer operator->() { return get(); }
        reference operator*() { return *get(); }

    private:
        friend class IntrusiveList;

        pointer get()
        {
            LinkNode* n = reinterpret_cast<LinkNode*>(mNode);
            return MemberToObject(n, M);
        }

        list_t* mNode;
    };

    class const_iterator
    {
    public:
        typedef const_iterator self;
        typedef T value_type;
        typedef ptrdiff_t difference_type;
        typedef const T* pointer;
        typedef const T& reference;
        typedef std::bidirectional_iterator_tag iterator_category;

        const_iterator() : mNode(NULL) {}
        explicit const_iterator(const list_t* node) : mNode(node) {}
        /* implicit */ const_iterator(iterator i) : mNode(i.mNode) {}  // NO_LINT

        bool operator==(const self& that) const { return mNode == that.mNode; }
        bool operator!=(const self& that) const { return !(*this == that); }

        self& operator++()
        {
            mNode = mNode->next;
            return *this;
        }

        self operator++(int)
        {
            self tmp(*this);
            ++*this;
            return tmp;
        }

        self& operator--() {
            mNode = mNode->prev;
            return *this;
        }

        self operator--(int)
        {
            self tmp(*this);
            --*this;
            return tmp;
        }

        pointer operator->() const { return get(); }
        reference operator*() const { return *get(); }

    private:
        friend class IntrusiveList;

        pointer get() const
        {
            // Need const_cast<>, because MemberToObject does not have
            // accept const object.
            LinkNode* n = reinterpret_cast<LinkNode*>(
                    const_cast<list_t*>(mNode));
            return MemberToObject(n, M);
        }

        const list_t* mNode;
    };

    typedef std::reverse_iterator<iterator> reverse_iterator;

    typedef IntrusiveList self;
    typedef T element_type;

    IntrusiveList() { list_init(&mList); }

    // Iterators
    iterator begin() { return iterator(mList.next); }
    const_iterator begin() const { return const_iterator(mList.next); }
    iterator end() { return iterator(&mList); }
    const_iterator end() const { return const_iterator(&mList); }
    iterator nodeiter(T* node) { return iterator(&(node->*M).mNode); }
    const_iterator nodeiter(T* node) const { return const_iterator(&(node->*M).mNode); }

    // Capacity
    bool empty() const { return list_empty(&mList); }
    size_t size() const { return std::distance(begin(), end()); }

    // Element access
    T* front() { return begin().get(); }
    const T* front() const { return const_cast<self*>(this)->front(); }

    T* back()
    {
        iterator i = end();
        --i;
        return i.get();
    }

    const T* back() const { return const_cast<self*>(this)->back(); }

    void link(iterator first, iterator last);

    // Modifiers
    iterator insert(iterator position, T* node);
    void erase(iterator position) { erase(position.get()); }
    void erase(T* node) { (node->*M).Unlink(); }
    void erase(iterator first, iterator last);

    void push_front(T* node) { insert(begin(), node); }
    void push_back(T* node) { insert(end(), node); }
    T* pop_front()
    {
        T* node = front();
        erase(node);
        return node;
    }
    T* pop_back()
    {
        T* node = back();
        erase(node);
        return node;
    }

    void move_to_front(T* node)
    {
        (node->*M).Unlink();
        list_add_tail(&(node->*M).mNode, mList.next);
    }
    void move_to_back(T* node)
    {
        (node->*M).Unlink();
        list_add_tail(&(node->*M).mNode, &mList);
    }

    void clear() { while (!empty()) { erase(begin()); } }

    void splice(iterator position, iterator i);
    void splice(iterator position, iterator first, iterator last);

    void swap(self* that)
    {
        list_t tmpNode;
        // replace the head node of 'this' list
        list_add_tail(&tmpNode, &this->mList);
        list_del(&this->mList);

        // replace the head node of 'that' list
        list_add_tail(&this->mList, &that->mList);
        list_del(&that->mList);

        // replace the head node of 'this' list
        list_add_tail(&that->mList, &tmpNode);
        list_del(&tmpNode);
    }

private:
    list_t mList;

    DISALLOW_COPY_AND_ASSIGN(IntrusiveList);
};

template <typename T, LinkNode T::*M>
typename IntrusiveList<T, M>::iterator
IntrusiveList<T, M>::insert(iterator position, T* value)
{
    list_t* node = &(value->*M).mNode;
    list_add_tail(node, position.mNode);
    return iterator(node);
}

template <typename T, LinkNode T::*M>
void IntrusiveList<T, M>::link(iterator first, iterator last)
{
    list_t* a = first.mNode;
    list_t* b = last.mNode;
    a->next = b;
    b->prev = a;
}

template <typename T, LinkNode T::*M>
void IntrusiveList<T, M>::erase(iterator first, iterator last)
{
    if (first == last)
    {
        return;
    }
    list_t* a = first.mNode->prev;
    list_t* b = last.mNode;
    a->next = b;
    b->prev = a;
}

template <typename T, LinkNode T::*M>
void IntrusiveList<T, M>::splice(iterator position, iterator i)
{
    iterator j = i;
    ++j;
    splice(position, i, j);
}

template <typename T, LinkNode T::*M>
void IntrusiveList<T, M>::splice(
        iterator position,
        iterator first,
        iterator last)
{
    if (first == last)
    {
        return;
    }
    if (position == last)
    {
        return;
    }
    // Before:
    //
    // A --> B (=position)
    //
    // C --> D (=first) --> ... --> E --> F (=last)
    //
    // After:
    //
    // A --> D (=first) --> ... --> E (=last) --> B (=position)
    //
    // C --> F
    list_t* a = position.mNode->prev;
    list_t* b = position.mNode;
    list_t* c = first.mNode->prev;
    list_t* d = first.mNode;
    list_t* e = last.mNode->prev;
    list_t* f = last.mNode;
    a->next = d;
    d->prev = a;
    e->next = b;
    b->prev = e;
    c->next = f;
    f->prev = c;
}

#endif  // _SRC_BASE_INTRUSIVE_LIST_H
