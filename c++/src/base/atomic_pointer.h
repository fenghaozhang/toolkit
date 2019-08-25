#ifndef _SRC_BASE_ATOMIC_POINTER_H
#define _SRC_BASE_ATOMIC_POINTER_H

inline void MemoryBarrier()
{
    // See http://gcc.gnu.org/ml/gcc/2003-04/msg01180.html for a discussion on
    // this idiom. Also see http://en.wikipedia.org/wiki/Memory_ordering.
    __asm__ __volatile__("" : : : "memory");
}

class AtomicPointer
{
public:
    AtomicPointer() { }
    explicit AtomicPointer(void* p) : mRep(p) {}
    inline void* NoBarrier_Load() const { return mRep; }
    inline void NoBarrier_Store(void* v) { mRep = v; }
    inline void* Acquire_Load() const
    {
        void* result = mRep;
        MemoryBarrier();
        return result;
    }
    inline void Release_Store(void* v)
    {
        MemoryBarrier();
        mRep = v;
    }

private:
    void* mRep;
};

#endif  // _SRC_BASE_ATOMIC_POINTER_H
