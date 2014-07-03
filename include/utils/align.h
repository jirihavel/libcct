#ifndef ALIGN_UTILS_H_INCLUDED
#define ALIGN_UTILS_H_INCLUDED

#include <cstdint>

namespace utils {

inline size_t alignSize(size_t size, size_t alignment)
{
    return (size+alignment-1)&~(alignment-1);
}

template<typename T>
inline T * alignPtr(T * ptr, uintptr_t alignment)
{
    union
    {
        T * p;
        uintptr_t u;
    } u;
    u.p = ptr;
    u.u = (u.u+alignment-1)&~(alignment-1);
    return u.p;
} 

}//namespace utils

#endif//ALIGN_UTILS_H_INCLUDED
