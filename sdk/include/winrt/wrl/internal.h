/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#ifndef _WRL_INTERNAL_H_
#define _WRL_INTERNAL_H_

#include <windows.h>

// TODO better place for this

#ifndef _MSC_VER

#if defined(__cplusplus) && (USE___UUIDOF == 0)
extern "C++" {
#if __cpp_constexpr >= 200704l && __cpp_inline_variables >= 201606L
__extension__ template<typename T> struct __mingw_uuidof_s;
__extension__ template<typename T> constexpr const GUID &__mingw_uuidof();
#else
__extension__ template<typename T> const GUID &__mingw_uuidof();
#endif
}
#endif

/* Macros for __uuidof template-based emulation */
#if defined(__cplusplus) && (USE___UUIDOF == 0)

#if __cpp_constexpr >= 200704l && __cpp_inline_variables >= 201606L
#define __CRT_UUID_DECL(type,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)    \
    extern "C++" {                                               \
    template<> struct __mingw_uuidof_s<type> {                   \
        static constexpr IID __uuid_inst = {                     \
            l,w1,w2, {b1,b2,b3,b4,b5,b6,b7,b8}                   \
        };                                                       \
    };                                                           \
    template<> constexpr const GUID &__mingw_uuidof<type>() {    \
        return __mingw_uuidof_s<type>::__uuid_inst;              \
    }                                                            \
    template<> constexpr const GUID &__mingw_uuidof<type*>() {   \
        return  __mingw_uuidof_s<type>::__uuid_inst;             \
    }                                                            \
    }
#else
#define __CRT_UUID_DECL(type,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)           \
    extern "C++" {                                                      \
    template<> inline const GUID &__mingw_uuidof<type>() {              \
        static const IID __uuid_inst = {l,w1,w2, {b1,b2,b3,b4,b5,b6,b7,b8}}; \
        return __uuid_inst;                                             \
    }                                                                   \
    template<> inline const GUID &__mingw_uuidof<type*>() {             \
        return __mingw_uuidof<type>();                                  \
    }                                                                   \
    }
#endif

#define __uuidof(type) __mingw_uuidof<__typeof(type)>()

#else

#define __CRT_UUID_DECL(type,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#endif
#endif

namespace Microsoft {
    namespace WRL {
        namespace Details {
            struct BoolStruct {
                int Member;
            };

            typedef int BoolStruct::* BoolType;

            inline void DECLSPEC_NORETURN RaiseException(HRESULT hr, DWORD flags = EXCEPTION_NONCONTINUABLE) throw() {
                ::RaiseException(static_cast<DWORD>(hr), flags, 0, NULL);
            }

            template <bool b, typename T = void>
            struct EnableIf {};

            template <typename T>
            struct EnableIf<true, T> {
                typedef T type;
            };
        }
    }
}

#endif
