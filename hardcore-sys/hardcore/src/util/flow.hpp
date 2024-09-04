#pragma once

#define HC_AS_TEXT(value) #value
#define HC_STRING(value) HC_AS_TEXT(value)

#ifdef NDEBUG

#include <utility>

#define HC_ASSERT(condition, expected) [[assumme(condition)]]

#define HC_UNREACHABLE(message) std::unreachable()

#else

#include <cstdlib>

#include <core/log.hpp>

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

/**
 * @brief Make an assertion.
 *
 * On builds where NDEBUG is defined, instead of asserting the condition expression, it turned into an [[assume]] clause.
 *
 * @param condition The boolean expression to be asserted.
 * @param expected The message given if the assertion fails
 */
#define HC_ASSERT(condition, expected)                                                                              \
if (!(condition)) {                                                                                                 \
    HC_ERROR(__FILE__ "(" HC_STRING(__LINE__) "): Assertion \"" HC_STRING(condition) "\" has failed: " expected);   \
    HC_DEBUGBREAK;                                                                                                  \
    std::abort();                                                                                                   \
}(0)

/**
 * @brief Mark unreachable code path.
 *
 * If execution goes through this path, the program will immediately abort.
 *
 * @param message Reason why the code path is not reachable.
 */
#define HC_UNREACHABLE(message)                                                           \
{                                                                                         \
    HC_ERROR(__FILE__ "(" HC_STRING(__LINE__) "): Entered unreachable code: " message);   \
    HC_DEBUGBREAK;                                                                        \
    std::abort();                                                                         \
}(0)

#endif // NDEBUG

/**
 * @brief Helper struct that keeps the compiler from immediately evaluating static asserts.
 *
 * @tparam T Any function or struct template parameter.
 */
template<typename T>
struct InstantiatedFalse : std::false_type {
};

/**
 * @brief Helper struct that keeps the compiler from immediately evaluating static asserts.
 *
 * @tparam T The type of the value.
 * @tparam V A const value.
 */
template<typename T, T V>
struct InstantiatedVarFalse : std::false_type {
};
