#ifndef TYPE_PUNING_H_INCLUDED
#define TYPE_PUNING_H_INCLUDED

#include <cstdint>

/** \brief Type puning - unsafe casts around type system.
 */
namespace pun {

template<typename T>
union ptr_punion
{
    T * p;
    std::intptr_t i;
    std::uintptr_t u;
};

/** \brief Pointer to signed integer.
 */
template<typename T>
inline std::intptr_t p2i(T * x) noexcept
{
    return ptr_punion<T>{.p = x}.i;
}

/** \brief Pointer to unsigned integer.
 */
template<typename T>
inline std::uintptr_t p2u(T * x) noexcept
{
    return ptr_punion<T>{.p = x}.u;
}

/** \brief Signed integer to pointer.
 */
template<typename T>
inline T * i2p(std::intptr_t x) noexcept
{
    return ptr_punion<T>{.i = x}.p;
}

/** \brief Unsigned integer to pointer.
 */
template<typename T>
inline T * u2p(std::uintptr_t x) noexcept
{
    return ptr_punion<T>{.u = x}.p;
}

using  intflt_t =  int32_t;
using uintflt_t = uint32_t;

union flt_punion
{
    float f;
    intflt_t i;
    uintflt_t u;
};

inline intflt_t f2i(float x) noexcept
{
    return flt_punion{.f = x}.i;
}

inline uintflt_t f2u(float x) noexcept
{
    return flt_punion{.f = x}.u;
}

using  intdbl_t =  int64_t;
using uintdbl_t = uint64_t;

union dbl_punion
{
    double f;
    intdbl_t i;
    uintdbl_t u;
};

inline intdbl_t f2i(double x) noexcept
{
    return dbl_punion{.f = x}.i;
}

inline uintdbl_t f2u(double x) noexcept
{
    return dbl_punion{.f = x}.u;
}

/** \brief Ordered conversion of float to uint.
 *
 * a < b <-> f2u_ord(a) < f2u_ord(b)
 */
inline uint32_t f2u_ord(float f) noexcept
{
    uint32_t const u = f2u(f);
	uint32_t const mask =
        -static_cast<int32_t>(u >> 31) | UINT32_C(0x80000000);
	return u ^ mask;
}

}//namespace pun

#endif//TYPE_PUNING_H_INCLUDED
