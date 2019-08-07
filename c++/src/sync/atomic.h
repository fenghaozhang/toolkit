#ifndef _SRC_SYNC_ATOMIC_H
#define _SRC_SYNC_ATOMIC_H

#include <stdint.h>

namespace detail
{

class AtomicDetailDefault
{
public:
    template<typename T>
    static void Set(volatile T* target, T value)
    {
        *target = value;
    }
    template<typename T>
    static T Exchange(volatile T* target, T value)
    {
        return __sync_lock_test_and_set(target, value);
    }
    template<typename T>
    static T ExchangeAdd(volatile T* target, T value)
    {
        return __sync_fetch_and_add(target, value);
    }
    template<typename T>
    static bool CompareExchange(volatile T* target, T exchange, T compare)
    {
        return __sync_bool_compare_and_swap(target, compare, exchange);
    }
};

template<int Size>
class AtomicDetail
{
public:
    template<typename T>
    static void Set(volatile T* target, T value);
    template<typename T>
    static bool CompareExchange(volatile T* target, T exchange, T compare);
    template<typename T>
    static T ExchangeAdd(volatile T* target, T value);
    template<typename T>
    static T Exchange(volatile T* target, T value);
};

template<>
class AtomicDetail<1> : public AtomicDetailDefault
{
};

template<>
class AtomicDetail<2> : public AtomicDetailDefault
{
};

template<>
class AtomicDetail<4> : public AtomicDetailDefault
{
};

#if __i386__

template<>
class AtomicDetail<8> : public AtomicDetailDefault
{
public:
    template<typename T>
    static void Set(volatile T* target, T value)
    {
        __sync_lock_test_and_set(target, value);
    }
};

#elif __x86_64__

template<>
class AtomicDetail<8> : public AtomicDetailDefault
{
};

template<>
class AtomicDetail<16>
{
public:
    template<typename T>
    static bool CompareExchange(volatile T* target, T exchange, T compare)
    {
        uint64_t *cmp = reinterpret_cast<uint64_t*>(&compare);
        uint64_t *with = reinterpret_cast<uint64_t*>(&exchange);
        bool result;
        __asm__ __volatile__
            (
             "lock cmpxchg16b %1\n\t"
             "setz %0"
             : "=q" (result), "+m" (*target), "+d" (cmp[1]), "+a" (cmp[0])
             : "c" (with[1]), "b" (with[0])
             : "cc"
            );
        return result;
    }
};

#else

#error ARCHITECT NOT SUPPORTED

#endif

}  // detail

// Compare the value stored in 'target' and 'compare', if they are equal,
// then set the 'exchange' into 'target' buffer.
// If the original value in '*target' equals to 'compare', return true
template<typename T>
inline bool AtomicCompareExchange(volatile T* target, T exchange, T compare)
{
    return detail::AtomicDetail<sizeof(T)>::CompareExchange(
        target,
        exchange,
        compare);
}

// Add 'value' to '*target', and return the original value.
template<typename T>
inline T AtomicExchangeAdd(volatile T* target, T value)
{
    return detail::AtomicDetail<sizeof(T)>::ExchangeAdd(target, value);
}

// Substract 'value' from '*target', and return the original value.
template<typename T>
inline T AtomicExchangeSub(volatile T* target, T value)
{
    return AtomicExchangeAdd(target, static_cast<T>(-value));
}

// Add 'value' to '*target', and return the new value in 'target'.
template<typename T>
inline T AtomicAdd(volatile T* target, T value)
{
    return AtomicExchangeAdd(target, value) + value;
}

// Substract 'value' from 'target', and return the new value in target.
template<typename T>
inline T AtomicSub(volatile T* target, T value)
{
    return AtomicExchangeSub(target, value) - value;
}

// Set 'value' into 'target', and return the old value in target.
template<typename T>
inline T AtomicExchange(volatile T* target, T value)
{
    return detail::AtomicDetail<sizeof(T)>::Exchange(target, value);
}

// Set 'value' into '*target'
template<typename T>
inline void AtomicSet(volatile T* target, T value)
{
    detail::AtomicDetail<sizeof(T)>::Set(target, value);
}

// Get the value in '*target'
template<typename T>
inline T AtomicGet(volatile T* target)
{
    T value = *target;
    return value;
}

// Add 1 to '*target', and return the new value in 'target'.
template<typename T>
inline T AtomicInc(volatile T* target)
{
    return AtomicAdd(target, static_cast<T>(1));
}

// Substract 1 from '*target', and return the new value in 'target'.
template<typename T>
inline T AtomicDec(volatile T* target)
{
    return AtomicSub(target, static_cast<T>(1));
}

#endif // _SRC_SYNC_ATOMIC_H
