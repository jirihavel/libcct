#ifndef UNDERLYING_VALUE_H
#define UNDERLYING_VALUE_H

#include <type_traits>

/** \brief Casts enum to its underlying_type
 *
 * Usefull for writing generic code with auto
 *
 * Limited to enums by std::enable_if
 */
template<typename T>
inline constexpr
typename std::enable_if<std::is_enum<T>::value,
        typename std::underlying_type<T>::type
    >::type underlying_value(T x) noexcept
{
    return static_cast<typename std::underlying_type<T>::type>(x);
}

#endif//UNDERLYING_VALUE_H
