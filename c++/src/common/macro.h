#ifndef _SRC_MACRO_H
#define _SRC_MACRO_H

#define DISALLOW_COPY_AND_ASSIGN(TypeName)  \
    TypeName(const TypeName&);              \
void operator=(const TypeName&);

#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#define COUNT_OF(x) (sizeof((x)) / sizeof(*(x)))

#define OFFSET_OF(type, member)                                         \
    (reinterpret_cast<size_t>(&static_cast<type*>(0)->member))

#define CONTAINER_OF(ptr, type, member)                                 \
    ({                                                                  \
     decltype(reinterpret_cast<type*>(0)->member)* __mptr = (ptr);      \
     reinterpret_cast<type*>(reinterpret_cast<char*>(__mptr) -          \
         OFFSET_OF(type, member));                                      \
     })

#define FOREACH(it, container)                                          \
    for (decltype((container).begin()) it = (container).begin();        \
        it != (container).end();                                        \
        ++it)

#define FOREACH_SAFE(it, container)                                     \
    for (decltype((container).begin()) it, __tmp = (container).begin(); \
        (it = __tmp) != (container).end();                              \
        ++__tmp)

#define ABORT()                                                         \
    do {                                                                \
        void (*abortptr)() = NULL;                                       \
        abortptr();                                                      \
    } while (false)

#endif  // _SRC_MACRO_H
