#ifndef _SRC_BASE_SLOT_VECTOR_H
#define _SRC_BASE_SLOT_VECTOR_H

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "src/common/macros.h"
#include "src/common/assert.h"
#include "src/sync/lock.h"

/**
 * @class SlotVector is a container class similar as a vector.  The storage space is arranged
 *        as an array of slots, in which each slot is an array of elements.
 *        The maximum number of slots and the element count in each slot are decided by
 *        the bit counts in template parameters and thus cannot be changed.
 *        The SlotVector can hold at most "1 << (_SLOT_INDEX_BITS + _VALUE_INDEX_BITS)" elements.
 *        As we use uint32_t as the index type of elements, the maximum allowed sum of
 *        _SLOT_INDEX_BITS and _VALUE_INDEX_BITS is 32.
 *        Different from vectors, this class can ensure that for all elements inside this
 *        container, the address will never change as long as the elements were not erased.
 *        Thus, it's safe to do things like: enlarge the container size in one thread, while
 *        reading existing elements in another thread.
 *        Note however that generally this class does not provide any other multi-thread safety
 *        ensurements, just like other STL containers.
 *
 *        Warning: As the SlotVector class contains a big array with length "1 << _SLOT_INDEX_BITS",
 *        usually the sizeof of a SlotVector instance could be very big (several hundred KB to several MB).
 *        So, it could be dangerous to directly allocate a SlotVector object on the stack.
 *        Instead, it's highly recommended that SlotVector objects (and objects of classes directly
 *        containing a SlotVector as a class member, like the template pool classes follows),
 *        should always be allocated on the heap space (by new operator).
 * @template _ValueType The type of the element value of slot vector.  This type should have a default
 *           constructor, and should be able to be copied (used in PushBack only).
 * @template _SLOT_INDEX_BITS The number of bits used in the index of slots.  The max possible value is 31.
 *           As there will be an array with allocated with length "1 << _SLOT_INDEX_BITS",
 *           it's not recommended to set it to a too big value.
 * @template _VALUE_INDEX_BITS The number of bits used in the index of elements in each slot.  The max possible
 *           value is 31.  As each slot will be allocated as an array with length
 *           "1 << _VALUE_INDEX_BITS", it's not recommended to set it to a too big value.
 */
template<typename _ValueType,
         uint32_t _SLOT_INDEX_BITS,
         uint32_t _VALUE_INDEX_BITS>
class SlotVector
{
public:
    typedef _ValueType ValueType;

    // This enum contains all kinds of constants that can be
    // determined in compile time.
    enum
    {
        SLOT_INDEX_BITS = _SLOT_INDEX_BITS,
        MAX_SLOT_NUM = 1U << SLOT_INDEX_BITS,
        VALUE_INDEX_BITS = _VALUE_INDEX_BITS,
        SLOT_LENGTH = 1U << VALUE_INDEX_BITS,
        VALUE_INDEX_MASK = SLOT_LENGTH - 1,
        MAX_LENGTH = 1ULL << (SLOT_INDEX_BITS + VALUE_INDEX_BITS),
        VALUE_SIZE = sizeof(ValueType),
        SLOT_SIZE = VALUE_SIZE << VALUE_INDEX_BITS,
    };

    /**
     * @brief The default constructor, which will initialize an empty slot vector.
     */
    SlotVector() : mSize(0)
    {
        // static_assert(SLOT_INDEX_BITS <= 31, "");
        // static_assert(VALUE_INDEX_BITS <= 31, "");
        // static_assert(SLOT_INDEX_BITS + VALUE_INDEX_BITS <= 32, "");
    }

    /**
     * @brief The default destructor.  All elements will be destructed and all slots will be deallocated.
     */
    ~SlotVector()
    {
        Clear(); // Clear() will deallocate all elements and slots.
    }

    /**
     * @brief Push a new element to the end of this slot vector.  The value type should be copyable.
     *        If the size of this slot vector has already reached the maximum capacity, a RuntimeError
     *        will be thrown.
     * @param value The element value to be added.
     */
    void PushBack(const ValueType& value)
    {
        ASSERT_DEBUG(mSize < MAX_LENGTH);
        uint32_t slotIndex = mSize >> VALUE_INDEX_BITS;
        uint32_t valueIndex = mSize & VALUE_INDEX_MASK;
        if (UNLIKELY(!valueIndex))
        {
            mSlots[slotIndex] = new char[SLOT_SIZE];
        }
        new (mValueSlots[slotIndex] + valueIndex) ValueType(value);
        ++mSize;
    }

    /**
     * @brief Pop the last element of this slot vector.  The element will be destructed first.
     *        If the slot vector is already empty, an RuntimeError will be throwm.
     */
    void PopBack()
    {
        ASSERT_DEBUG(mSize > 0);
        --mSize;
        uint32_t slotIndex = mSize >> VALUE_INDEX_BITS;
        uint32_t valueIndex = mSize & VALUE_INDEX_MASK;
        mValueSlots[slotIndex][valueIndex].~ValueType();
        if (UNLIKELY(!valueIndex))
        {
            delete[] mSlots[slotIndex];
        }
    }

    /**
     * @brief Return the reference of the last element of this slot vector, the non-const version.
     *        Note that this method will not check the size of this slot vector, so the behavior could be
     *        unexceptable if this slot vector is empty.
     * @return Return the reference of the last element of this slot vector.
     */
    ValueType& Back()
    {
        return (*this)[mSize - 1];
    }

    /**
     * @brief Return the reference of the last element of this slot vector, the const version.
     *        Note that this method will not check the size of this slot vector, so the behavior could be
     *        unexceptable if this slot vector is empty.
     * @return Return the reference of the last element of this slot vector.
     */
    const ValueType& Back() const
    {
        return (*this)[mSize - 1];
    }

    /**
     * @brief Overload of the operator[] which will return the reference of the n-th element of this slot vector,
     *        the non-const version.  Note that this method will not make out-of-range check on the given index,
     *        so the behavior could be unexceptable if the given index exceeds the size of this slot vector.
     * @param index The index of the element to be get.  The index starts from 0.
     * @return Return the reference of the n-th element of this slot vector.
     */
    ValueType& operator[](uint32_t index)
    {
        return mValueSlots[index >> VALUE_INDEX_BITS][index & VALUE_INDEX_MASK];
    }

    /**
     * @brief Overload of the operator[] which will return the reference of the n-th element of this slot vector,
     *        the const version.  Note that this method will not make out-of-range check on the given index,
     *        so the behavior could be unexceptable if the given index exceeds the size of this slot vector.
     * @param index The index of the element to be get.  The index starts from 0.
     * @return Return the reference of the n-th element of this slot vector.
     */
    const ValueType& operator[](uint32_t index) const
    {
        return mValueSlots[index >> VALUE_INDEX_BITS][index & VALUE_INDEX_MASK];
    }

    /**
     * @brief Get the number of elements contained in this slot vector.
     * @return Return the number of elements contained in this slot vector.
     */
    uint64_t Size() const
    {
        return mSize;
    }

    /**
     * @brief Clear this slot vector, which will destruct all elements in this slot vector, and dealloc the memory
     *        for all slots.
     */
    void Clear()
    {
        Resize(0);
    }

    /**
     * @brief Change the size of this slot vector to a new value.  If the new size is larger than the current size,
     *        New elements will be created with their default constructors.  If the new size is smaller than the
     *        current size, the removed elements will be destructed.
     * @param newSize The new size of this slot vector.  If the new size exceeds the max capacity, a RuntimeError
     *                will be thrown.
     */
    void Resize(uint64_t newSize)
    {
        ASSERT(newSize <= MAX_LENGTH);
        if (LIKELY(newSize > mSize))
        {
            char* addr = mSlots[mSize >> VALUE_INDEX_BITS] +
                         VALUE_SIZE * (mSize & VALUE_INDEX_MASK);
            for (uint64_t i = mSize; i < newSize; ++i, addr += VALUE_SIZE)
            {
                if (UNLIKELY(!(i & VALUE_INDEX_MASK)))
                { // beginning of a new slot
                    addr = mSlots[i >> VALUE_INDEX_BITS] = new char[SLOT_SIZE];
                }
                new (addr) ValueType();
            }
        }
        else if (newSize < mSize)
        {
            uint64_t i = mSize - 1;
            ValueType* addr = mValueSlots[i >> VALUE_INDEX_BITS] +
                              (i & VALUE_INDEX_MASK);
            for (; ; --i, --addr)
            {
                addr->~ValueType();
                if (UNLIKELY(!(i & VALUE_INDEX_MASK)))
                { // removed last element in a slot
                    delete[] mSlots[i >> VALUE_INDEX_BITS];
                    if (i)
                    {
                        addr = mValueSlots[(i >> VALUE_INDEX_BITS) - 1] +
                               SLOT_LENGTH;
                    }
                }
                if (i == newSize)
                {
                    break;
                }
            }
        }
        mSize = newSize;
    }

    /**
     * @brief Get the brief memory space occupied by this slot vector.  This includes the memory space
     *        for the slots, and the memory space of the elements.
     * @return Get the brief memory space occupied by this slot vector.
     */
    uint64_t GetMemorySize() const
    {
        return sizeof(*this) +
               ((mSize + SLOT_LENGTH - 1) >> VALUE_INDEX_BITS) * SLOT_SIZE;
    }

private:
    uint64_t mSize;
    // Use a union here so that it's easy to access this pointer vector as
    // either char* pointers or ValueType* pointers without type cast.
    union
    {
        char* mSlots[MAX_SLOT_NUM];
        ValueType* mValueSlots[MAX_SLOT_NUM];
    };

private:
    // Disallow copies.
    SlotVector(const SlotVector&);
    SlotVector& operator=(const SlotVector&);
};

template<typename _ValueType,
         uint32_t _SLOT_INDEX_BITS,
         uint32_t _VALUE_INDEX_BITS,
         typename LockType = stone::SimpleMutex>
class SlotVectorPool
{
public:
    typedef _ValueType ValueType;

    enum
    {
        SLOT_INDEX_BITS = _SLOT_INDEX_BITS,
        MAX_SLOT_NUM = 1U << SLOT_INDEX_BITS,
        VALUE_INDEX_BITS = _VALUE_INDEX_BITS,
        SLOT_LENGTH = 1U << VALUE_INDEX_BITS,
        VALUE_INDEX_MASK = SLOT_LENGTH - 1,
        MAX_LENGTH = 1ULL << (SLOT_INDEX_BITS + VALUE_INDEX_BITS),
        VALUE_SIZE = sizeof(ValueType),
        SLOT_SIZE = VALUE_SIZE << VALUE_INDEX_BITS,
    };

    SlotVectorPool()
    {
        STATIC_ASSERT(VALUE_SIZE >= sizeof(uint32_t));
        STATIC_ASSERT((uint64_t)MAX_LENGTH < (uint32_t)(-1));
        STATIC_ASSERT(SLOT_LENGTH >= 4);
        mSlotVector.Resize(SLOT_LENGTH);
        mNextFreeItemIndex = 0;
        mNewFreeItemIndex  = 1;
    }

    ~SlotVectorPool()
    {
        //TODO, shuqi.zsq enable it later, currently will assert
#if 0
        typename LockType::Locker lock(mMutex);
        size_t freeCnt = 1;
        uint32_t itemIndex = mNextFreeItemIndex;
        while (itemIndex != 0)
        {
            ++freeCnt;
            ValueType* item = GetItem(itemIndex);
            itemIndex = *(reinterpret_cast<uint32_t*>(item));
        }
        ASSERT(freeCnt == mNewFreeItemIndex);
#endif
    }

    ValueType* AllocItem(uint32_t* blockItemIndex)
    {
        typename LockType::Locker lock(mMutex);
        uint32_t itemIndex = 0;
        ValueType* item = NULL;
        if (LIKELY(mNextFreeItemIndex != 0))
        {
            itemIndex = mNextFreeItemIndex;
            item = GetItem(itemIndex);
            mNextFreeItemIndex = *(reinterpret_cast<uint32_t*>(item));
        }
        else if (LIKELY(mNewFreeItemIndex < mSlotVector.Size()))
        {
            itemIndex = mNewFreeItemIndex++;
            item = GetItem(itemIndex);
        }
        else
        {
            mSlotVector.Resize(mSlotVector.Size() + SLOT_LENGTH);
            itemIndex = mNewFreeItemIndex++;
            item = GetItem(itemIndex);
        }
        new (item) ValueType();
        *blockItemIndex = itemIndex;
        return item;
    }

    ValueType* GetItem(uint32_t blockItemIndex)
    {
        return reinterpret_cast<ValueType*>(&mSlotVector[blockItemIndex]);
    }

    void DeallocItem(uint32_t blockItemIndex)
    {
        typename LockType::Locker lock(mMutex);
        ValueType* value = GetItem(blockItemIndex);
        value->~ValueType();
        *(reinterpret_cast<uint32_t*>(value)) = mNextFreeItemIndex;
        mNextFreeItemIndex = blockItemIndex;
    }

    bool IsItemIndexValid(uint32_t blockItemIndex)
    {
        return blockItemIndex < mNewFreeItemIndex;
    }

private:
    mutable LockType mMutex;
    uint32_t mNextFreeItemIndex;
    uint32_t mNewFreeItemIndex;
    struct ValueContainer
    {
        char data[VALUE_SIZE < sizeof(uint32_t) ? sizeof(uint32_t) : VALUE_SIZE];
    };
    SlotVector<ValueContainer, _SLOT_INDEX_BITS, _VALUE_INDEX_BITS> mSlotVector;
};

#endif  // _SRC_BASE_SLOT_VECTOR_H
