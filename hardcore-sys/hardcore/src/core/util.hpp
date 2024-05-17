#pragma once

#include <cstdint>

#include "log.hpp"

#define HC_AS_TEXT(value) #value
#define HC_STRING(value) HC_AS_TEXT(value)

#ifdef NDEBUG

#define HC_ASSERT(condition, expected)

#else

#include <cstdlib>

// Adapted from https://github.com/nemequ/portable-snippets/blob/master/debug-trap/debug-trap.h
#if defined(__has_builtin) && !defined(__ibmxl__)

#if __has_builtin(__builtin_debugtrap)
#define HC_DEBUGBREAK __builtin_debugtrap()
#elif __has_builtin(__debugbreak)
#define HC_DEBUGBREAK __debugbreak()
#endif

#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)

#define HC_DEBUGBREAK __debugbreak()

#elif defined(__ARMCC_VERSION)

#define HC_DEBUGBREAK __breakpoint(42)

#elif __has_include (<signal.h>)

#include <signal.h>
#if defined(SIGTRAP)
#define HC_DEBUGBREAK raise(SIGTRAP)
#else
#define HC_DEBUGBREAK
#endif // defined(SIGTRAP)

#else
#define HC_DEBUGBREAK
#endif

#define HC_ASSERT(condition, expected)                                                                              \
if (!(condition)) {                                                                                                 \
    HC_ERROR(__FILE__ "(" HC_STRING(__LINE__) "): Assertion \"" HC_STRING(condition) "\" has failed: " expected);   \
    HC_DEBUGBREAK;                                                                                                  \
    std::abort();                                                                                                   \
}(0)

#endif // NDEBUG

typedef std::uint8_t u8; //!< Short alias for a 8 bit unsigned integer.
typedef std::uint16_t u16; //!< Short alias for a 16 bit unsigned integer.
typedef std::uint32_t u32; //!< Short alias for a 32 bit unsigned integer.
typedef std::uint64_t u64; //!< Short alias for a 64 bit unsigned integer.

typedef std::int8_t i8; //!< Short alias for a 8 bit integer.
typedef std::int16_t i16; //!< Short alias for a 16 bit integer.
typedef std::int32_t i32; //!< Short alias for a 32 bit integer.
typedef std::int64_t i64; //!< Short alias for a 64 bit integer.

typedef std::size_t Sz; //!< Short alias for the standard size type. (`std::size_t`)
