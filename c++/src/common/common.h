#ifndef _SRC_COMMON_COMMON_H
#define _SRC_COMMON_COMMON_H

template <typename T, typename M>
T* MemberToObject(M* member, M T::*mem_ptr)
{
    size_t offset = reinterpret_cast<size_t>(
            &((reinterpret_cast<T*>(0))->*mem_ptr));
    return reinterpret_cast<T*>(
            reinterpret_cast<char*>(member) - offset);
}

#endif  // _SRC_COMMON_COMMON_H
