// Author: WangZhan -> wangzhan.1985@gmail.com
#pragma once
#include <iostream>

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&); \
    void operator= (const TypeName&)


// - The array size is (bool(expr) ? 1 : -1), instead of simply
//
//     ((expr) ? 1 : -1).
template <bool>
struct CompileZhanAssert {
};

#undef COMPILE_ZhanAssert
#define COMPILE_ZhanAssert(expr, msg) \
    typedef CompileZhanAssert<(bool(expr))> msg[bool(expr) ? 1 : -1]


#ifdef _UNICODE
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif

#ifdef _DEBUG
#define ZhanAssert(exp) \
do { \
if (!(exp)) \
    DebugBreak(); \
    } while (0)
#else
#define ZhanAssert(exp)
#endif // DEBUG


