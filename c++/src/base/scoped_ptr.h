#ifndef _SRC_BASE_SCOPED_PTR_H
#define _SRC_BASE_SCOPED_PTR_H

#include <assert.h>
#include <stdlib.h>
#include "src/common/uncopyable.h"

template<typename T>
class ScopedPtrDelete
{
public:
    void operator()(T* p)
    {
        delete p;
    }
};

template<typename T>
class ScopedPtrDeleteArray
{
public:
    void operator()(T* p)
    {
        delete[] p;
    }
};

class ScopedPtrFree
{
public:
    void operator()(void* p)
    {
        ::free(p);
    }
};

/**
 * scoped_ptr mimics a built-in pointer except that it guarantees deletion
 * of the object pointed to, either on destruction of the scoped_ptr or via
 * an explicit reset(). scoped_ptr is a simple solution for simple needs;
 * use shared_ptr or std::auto_ptr if your needs are more complex.
 * You are encouraged to manage the life cycle of the objects explicitly,
 * scoped_ptr is a handy tool to bind the life time of an object to the life
 * time of a statement "scope", save the user from explicit delete/free call.
 */
template<class T, class Destructor = ScopedPtrDelete<T> >
class scoped_ptr : private Uncopyable
{
    typedef scoped_ptr<T, Destructor> this_type;
public:
    // Provides the type of the stored pointer.
    typedef T element_type;

    /**
     * Constructs a scoped_ptr, storing a copy of ptr, which must have been
     * allocated via a C++ new expression or be NULL if Destructor is
     * ScopedPtrDelete, and ptr could be a malloc-ed pointer if Destructor
     * is ScopedPtrFree. T is not required be a complete type.
     */
    explicit scoped_ptr(T* ptr = NULL) : mPtr(ptr) {}

    // It equals to this->reset(ptr).
    scoped_ptr& operator=(T* ptr)
    {
        reset(ptr);
        return *this;
    }

    /**
     * Destroys the object pointed to by the stored pointer, if any, as if
     * by using delete this->get()/::free(this->get()).
     */
    ~scoped_ptr()
    {
        Destructor destructor;
        destructor(mPtr);
    }

    /**
     * Deletes the object pointed to by the stored pointer and then stores
     * a copy of ptr.
     * Pay attention! the ptr MUST NOT break the destruct limit. That's to
     * say, the ptr assigned to scoped_ptr MUST be created with C++ new
     * expression if the Destructor is ScopedPtrDeleteArray. And when
     * Destructor is ScopedPtrFree, it MUST BE alloced with malloc/alloc etc.
     */
    void reset(T* ptr = NULL)
    {
        assert(ptr == NULL || ptr != mPtr);
        this_type(ptr).swap(*this);
    }

    /**
     * Returns a reference to the object pointed to by the stored pointer.
     * Behavior is undefined if the stored pointer is NULL.
     */
    T& operator*() const  // NO_LINT(runtime/operator)
    {
        assert(mPtr);
        return *mPtr;
    }

    /**
     * Returns the stored pointer. Behavior is undefined if the stored
     * pointer is NULL.
     */
    T* operator->() const
    {
        assert(mPtr);
        return mPtr;
    }
    // Returns the stored pointer. T need not be a complete type.
    T* get() const
    {
        return mPtr;
    }
    // Checks whether the stored pointer is NOT NULL.
    operator bool() const
    {
        return mPtr != NULL;
    }

    /**
     * Exchanges the contents of the two smart pointers. T need not be a
     * complete type.
     */
    void swap(this_type& other)
    {
        T* tmp = other.mPtr;
        other.mPtr = mPtr;
        mPtr = tmp;
    }

    // Resets the saved pointer to NULL, and return the old value.
    T* release()
    {
        T* old = mPtr;
        mPtr = 0;
        return old;
    }

private:
    T* mPtr;

    // With the meaning of scoped_ptr, the following operations make no sense
    // so disable them.
    // disable copy constructor, and assign operator
    template<typename T1, typename Deletor1>
    scoped_ptr(const scoped_ptr<T1, Deletor1>& other);
    template<typename T1, typename Deletor1>
    scoped_ptr& operator=(const scoped_ptr<T1, Deletor1>& other);

    // disable the compare operations
    template<typename T1, typename Deletor1>
    bool operator==(const scoped_ptr<T1, Deletor1>&) const;
    template<typename T1, typename Deletor1>
    bool operator!=(const scoped_ptr<T1, Deletor1>&) const;
};

/**
 * scoped_array extends scoped_ptr to arrays. Deletion of the array pointed to
 * is guaranteed, either on destruction of the scoped_array or via an explicit
 * reset(). Use shared_array or std::vector if your needs are more complex.
 * You are encouraged to manage the life cycle of the objects explicitly,
 * scoped_array is a handy tool to bind the life time of an object to the life
 * time of a statement "scope", save the user from explicit delete/free call.
 */
template<typename T, typename Destructor = ScopedPtrDeleteArray<T> >
class scoped_array : Uncopyable
{
    typedef scoped_array<T, Destructor> this_type;
public:
    // Provides the type of the stored pointer.
    typedef T element_type;

    /**
     * Constructs a scoped_array, storing a copy of ptr, which must have
     * been allocated via a C++ new[] expression or be 0 if Destructor is
     * ScopedPtrDelete, and ptr could be a malloc-ed pointer if Destructor
     * is ScopedPtrFree. T is not required be a complete type.
     */
    explicit scoped_array(T* ptr = NULL) : mPtr(ptr) {}

    // It equals to this->reset(ptr).
    scoped_array& operator=(T* ptr)
    {
        reset(ptr);
        return *this;
    }

    /**
     * Destroys the object pointed to by the stored pointer, if any, as if
     * by using delete[] this->get()/::free(this->get()).
     */
    ~scoped_array()
    {
        Destructor destructor;
        destructor(mPtr);
    }

    /**
     * Deletes the objects pointed to by the stored pointer and then stores
     * a copy of ptr.
     * Pay attention! the ptr MUST NOT break the destruct limit. That's to
     * say, the ptr assigned to scoped_array MUST be created with C++ new
     * expression if the Destructor is ScopedPtrDelete. And when Destructor
     * is ScopedPtrFree, it MUST BE alloced with malloc/alloc etc.
     */
    void reset(T* ptr = NULL)
    {
        assert(ptr == NULL || ptr != mPtr);
        this_type(ptr).swap(*this);
    }

    /**
     * Returns the i-th elements of the array the stored pointer point to.
     * Behavior is undefined if the stored pointer is 0.
     */
    T& operator[](int i) const
    {
        assert(mPtr);
        return mPtr[i];
    }
    // Returns the stored pointer. T need not be a complete type.
    T* get() const
    {
        return mPtr;
    }
    // Checks whether the stored pointer is NOT NULL.
    operator bool() const
    {
        return mPtr != NULL;
    }

    /**
     * Exchanges the contents of the two smart pointers. T need not be a
     * complete type.
     */
    void swap(this_type& other)
    {
        T* tmp = other.mPtr;
        other.mPtr = mPtr;
        mPtr = tmp;
    }

    // Resets the saved pointer to NULL, and return the old value.
    T* release()
    {
        T* old = mPtr;
        mPtr = NULL;
        return old;
    }

private:
    T* mPtr;

    // Disable the following operations.
    template<typename T1, typename Deletor1>
    scoped_array(const scoped_array<T1, Deletor1>& other);
    template<typename T1, typename Deletor1>
    scoped_array& operator=(const scoped_array<T1, Deletor1>& other);

    template<typename T1, typename Deletor1>
    bool operator==(const scoped_array<T1, Deletor1>&) const;
    template<typename T1, typename Deletor1>
    bool operator!=(const scoped_array<T1, Deletor1>&) const;
};

#endif  // _SRC_BASE_SCOPED_PTR_H
