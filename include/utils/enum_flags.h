/*
Copyright (c) 2013, Yuri Yaryshev (aka Lord Odin)

The MIT License (MIT)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
/*
Modified 2014 by Jiří Havel
- std::underlying_type used instead of macro parameter
- function underlying_value(T x) for cast to underlying_type
- constexpr functions
*/

/*
Usage sample:

#include "enum_flags.h"

ENUM_FLAGS(foo_t)
enum class foo_t
	{
	 none			= 0x00
	,a				= 0x01
	,b				= 0x02
	};
	
ENUM_FLAGS(foo2_t)
enum class foo2_t
	{
	 none			= 0x00
	,d				= 0x01
	,e				= 0x02
	};	

int _tmain(int argc, _TCHAR* argv[])
	{
	if(flags(foo_t::a & foo_t::b)) {};
	// if(flags(foo2_t::d & foo_t::b)) {};	// Type safety test - won't compile if uncomment
	};

*/

#ifndef __ENUM_FLAGS_H__
#define __ENUM_FLAGS_H__

#include "underlying_value.h"

/*
Use this line before header, if you don't want flags(T x) function to be implemented for your enum.
In that case you can use less polluting underlying_value(T x) with implicit conversion to bool.
#define USE_ENUM_FLAGS_FUNCTION 0
*/

#ifndef USE_ENUM_FLAGS_FUNCTION
# define USE_ENUM_FLAGS_FUNCTION 0
#endif

#define ENUM_FLAGS_OPERATORS(T) \
inline constexpr T operator ~ (T x) { return static_cast<T>(~underlying_value(x)); }; \
inline constexpr T operator & (T x, T y) { return static_cast<T>(underlying_value(x) & underlying_value(y)); }; \
inline constexpr T operator | (T x, T y) { return static_cast<T>(underlying_value(x) | underlying_value(y)); }; \
inline constexpr T operator ^ (T x, T y) { return static_cast<T>(underlying_value(x) ^ underlying_value(y)); }; \
inline T & operator &= (T & x, T y) { x = x & y; return x; }; \
inline T & operator |= (T & x, T y) { x = x | y; return x; }; \
inline T & operator ^= (T & x, T y) { x = x ^ y; return x; };

#if(USE_ENUM_FLAGS_FUNCTION)
# define ENUM_FLAGS(T) enum class T; ENUM_FLAGS_OPERATORS(T) \
inline constexpr bool flags(T x) { return underlying_value(x); }; \
enum class T
#else
# define ENUM_FLAGS(T) enum class T; ENUM_FLAGS_OPERATORS(T) \
enum class T
#endif

#endif
