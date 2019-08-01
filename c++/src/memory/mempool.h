#ifndef _SRC_MEMORY_MEMPOOL_H
#define _SRC_MEMORY_MEMPOOL_H

#include "src/base/call_traits.h"
#include "src/common/macro.h"

class MemPool
{
public:
    MemPool() { init(); }

    ~MemPool() { destroy(); }

    template <typename T>
    T* New()
    {
        return new T;
    }

    template <typename T, typename Arg1>
    // T* New(typename call_traits<Arg1>::param_type arg1)
    T* New(const Arg1& arg1)
    {
        return new T(arg1);
    }

private:
    void init();
    void destroy();

    DISALLOW_COPY_AND_ASSIGN(MemPool);
};

void MemPool::init()
{
    // TODO(allen.zfh): do something
    return;
}

void MemPool::destroy()
{
    // TODO(allen.zfh): do something
    return;
}

#endif  // _SRC_MEMORY_MEMPOOL_H
