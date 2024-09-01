#pragma once

#include <bit>
#include <cstdint>
#include <variant>

#include "log.hpp"

#define HC_AS_TEXT(value) #value
#define HC_STRING(value) HC_AS_TEXT(value)

#ifdef NDEBUG

#define HC_ASSERT(condition, expected) [[assumme(condition)]]

#define HC_UNREACHABLE(message) std::unreachable()

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

#define KIBIBYTES(value) ((value) << 10)
#define MEBIBYTES(value) (KIBIBYTES(value) << 10)
#define GIBIBYTES(value) (MEBIBYTES(value) << 10)

typedef std::uint8_t u8; //!< Short alias for a 8 bit unsigned integer.
typedef std::uint16_t u16; //!< Short alias for a 16 bit unsigned integer.
typedef std::uint32_t u32; //!< Short alias for a 32 bit unsigned integer.
typedef std::uint64_t u64; //!< Short alias for a 64 bit unsigned integer.

typedef std::int8_t i8; //!< Short alias for a 8 bit integer.
typedef std::int16_t i16; //!< Short alias for a 16 bit integer.
typedef std::int32_t i32; //!< Short alias for a 32 bit integer.
typedef std::int64_t i64; //!< Short alias for a 64 bit integer.

typedef std::size_t Sz; //!< Short alias for the standard size type. (`std::size_t`)

/**
 * @brief An uncopyable value.
 *
 * The inner value of this object may only be moved from one instance to another.
 * When a move occurs, the object that got moved from is assigned the specified default value.
 *
 * @tparam T The type of the inner value.
 * @tparam DEFAULT The default value assigned by the default constructor, move constructor and move assignment.
 */
template<typename T, T DEFAULT>
class Uncopyable {
public:
    Uncopyable() = default;

    /**
     * @brief Implicit value constructor.
     *
     * @param value The value to assign to this uncopyable variable.
     */
    Uncopyable(const T &value) : value(value) {} // NOLINT intentionally implicit

    Uncopyable(const Uncopyable &) = delete;

    Uncopyable &operator=(const Uncopyable &) = delete;

    Uncopyable(Uncopyable &&other) noexcept: value(std::exchange(other.value, DEFAULT)) {}

    Uncopyable &operator=(Uncopyable &&other) noexcept {
        this->value = std::exchange(other.value, DEFAULT);
    }

    operator T() const { return value; } // NOLINT intentionally implicit

    operator T &() { return value; } // NOLINT intentionally implicit

private:
    T value = DEFAULT;
};

/**
 * @brief A value which cannot be copied and cannot be moved into a value that has already been assigned.
 *
 * Similar to an `Uncopyable`.
 * The inner value of this object may only be moved from one instance to another.
 * When a move occurs, the object that got moved from is assigned the specified default value.
 *
 * Unlike an `Uncopyable` when moving into a value that has already been assigned, the program SHOULD crash. "Should"
 * because this type is only used internally, any related errors should be easily caught in debug builds.
 *
 * @tparam T The type of the inner value.
 * @tparam DEFAULT The default value assigned by the default constructor, move constructor and move assignment.
 */
template<typename T, T DEFAULT>
class ExternalHandle {
public:
    ExternalHandle() = default;

    /**
     * @brief Implicit value constructor.
     *
     * @param value The value to assign to this external handle.
     */
    ExternalHandle(const T &value) : value(value) {} // NOLINT intentionally implicit

    ~ExternalHandle() {
        HC_ASSERT(this->value == DEFAULT, "Inner value must be externally cleaned up");
    }

    ExternalHandle(const ExternalHandle &) = delete;

    ExternalHandle &operator=(const ExternalHandle &) = delete;

    ExternalHandle(ExternalHandle &&other) noexcept: value(std::exchange(other.value, DEFAULT)) {}

    ExternalHandle &operator=(ExternalHandle &&other) noexcept {
        HC_ASSERT(this->value == DEFAULT, "Inner value cannot be overwritten if already assigned");
        this->value = std::exchange(other.value, DEFAULT);
        return *this;
    }

    operator T() const { return value; } // NOLINT intentionally implicit

    operator T &() { return value; } // NOLINT intentionally implicit

    /**
     * @brief "Destroy" this handle by setting it to the default value.
     */
    void destroy() {
        this->value = DEFAULT;
    }

private:
    T value = DEFAULT;
};

/**
 * @brief A class representing the result of some operation, typically returned by functions.
 *
 * This class loosely mimics Rust's Result enum, to facilitate error handling.
 *
 * @tparam Ok The type of the value returned on success.
 * @tparam Error The error type, returned by unsuccessful operations.
 */
template<typename Ok, typename Error>
class Result {
public:
    // No default constructor.
    Result() = delete;

    /**
     * @brief Create a new successful `Result` from an `Ok` rvalue.
     *
     * @param ok The value from which the `Result` is created.
     *
     * @return The created result.
     */
    static constexpr Result ok(Ok &&ok) {
        return Result(std::variant<Ok, Error>(std::in_place_index<0>, std::move(ok)));
    }

    /**
     * @brief Create a new unsuccessful `Result` from an `Error` rvalue.
     *
     * @param ok The value from which the `Result` is created.
     *
     * @return The created result.
     */
    static constexpr Result err(Error &&err) {
        return Result(std::variant<Ok, Error>(std::in_place_index<1>, std::move(err)));
    }

    /**
     * @brief Get the success value.
     *
     * @throw std::bad_variant_access If this `Result` does not represent a success.
     *
     * @return The success value, as a const reference.
     */
    constexpr const Ok &ok() const &{
        return std::get<0>(this->value);
    }

    /**
     * @brief Get the success value.
     *
     * @throw std::bad_variant_access If this `Result` does not represent a success.
     *
     * @return The success value, as an rvalue.
     */
    constexpr Ok &&ok() &&{
        return std::get<0>(std::move(this->value));
    }

    /**
     * @brief Get the error value.
     *
     * @throw std::bad_variant_access If this `Result` does not represent an error.
     *
     * @return The error value, as a const reference.
     */
    constexpr const Error &err() const &{
        return std::get<1>(this->value);
    }

    /**
     * @brief Get the error value.
     *
     * @throw std::bad_variant_access If this `Result` does not represent an error.
     *
     * @return The error value, as an rvalue.
     */
    constexpr Error &&err() &&{
        return std::get<1>(std::move(this->value));
    }

    /**
     * @brief Query the type of value this `Result` is.
     *
     * @return *true* if this `Result` represents a success, false otherwise.
     */
    constexpr explicit operator bool() const noexcept {
        return this->value.index() == 0;
    }

private:
    /**
     * @brief Basic constructor to avoid needless move assignments.
     *
     * @param value The inner value of the `Result`.
     */
    explicit Result(std::variant<Ok, Error> &&value) : value(std::move(value)) {}

    // std::variant does all the heavy lifting for destruction, assignments, etc..
    std::variant<Ok, Error> value; //!< The inner result value.
};

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
 * @tparam T A size value.
 */
template<Sz T>
struct InstantiatedSizeFalse : std::false_type {
};

template<class T>
concept Integral = std::is_integral<T>::value; //!< An integral number type.

static constexpr Sz reversed_bytes_size = 256; //!< The size of the `reversed_bytes` array.
/**
 * @brief An array containing reversed bit values for any byte.
 *
 * Indexing into this array with any byte value will yield that same byte with its bits reversed.
 */
static constexpr std::array<u8, reversed_bytes_size> reversed_bytes{[]() constexpr {
    auto res = std::array<u8, reversed_bytes_size>();
    for (Sz i = 0; i < reversed_bytes_size; ++i) {
        u8 byte = i;
        u8 count = 8;
        u8 reversed_byte = 0;
        while (count) {
            reversed_byte = (reversed_byte << 1) | (byte & 1);
            byte = byte >> 1;
            count--;
        }
        res[i] = reversed_byte;
    }
    return res;
}()};

/**
 * @brief Reverse the bits of an arbitrarily sized byte array.
 *
 * While specializations may implement any arbitrary size, this function is meant to be used only by `reverse_bits`,
 * and may not im implemented for sizes irrelevant to that function.
 *
 * @tparam N Size of the array.
 * @param bytes The array to be reversed.
 */
template<Sz N>
constexpr void reverse_byte_array([[maybe_unused]] std::array<u8, N> &bytes) noexcept {
    static_assert(InstantiatedSizeFalse<N>::value, "Impossible number byte size");
}

template<>
[[maybe_unused]] constexpr void reverse_byte_array<1>(std::array<u8, 1> &bytes) noexcept {
    bytes[0] = reversed_bytes[bytes[0]];
}

template<>
[[maybe_unused]] constexpr void reverse_byte_array<2>(std::array<u8, 2> &bytes) noexcept {
    u8 tmp;
    tmp = reversed_bytes[bytes[0]];
    bytes[0] = reversed_bytes[bytes[1]];
    bytes[1] = tmp;
}

template<>
[[maybe_unused]] constexpr void reverse_byte_array<4>(std::array<u8, 4> &bytes) noexcept {
    u8 tmp;
    tmp = reversed_bytes[bytes[0]];
    bytes[0] = reversed_bytes[bytes[3]];
    bytes[3] = tmp;
    tmp = reversed_bytes[bytes[1]];
    bytes[1] = reversed_bytes[bytes[2]];
    bytes[2] = tmp;
}

template<>
[[maybe_unused]] constexpr void reverse_byte_array<8>(std::array<u8, 8> &bytes) noexcept {
    u8 tmp;
    tmp = reversed_bytes[bytes[0]];
    bytes[0] = reversed_bytes[bytes[7]];
    bytes[7] = tmp;
    tmp = reversed_bytes[bytes[1]];
    bytes[1] = reversed_bytes[bytes[6]];
    bytes[6] = tmp;
    tmp = reversed_bytes[bytes[2]];
    bytes[2] = reversed_bytes[bytes[5]];
    bytes[5] = tmp;
    tmp = reversed_bytes[bytes[3]];
    bytes[3] = reversed_bytes[bytes[4]];
    bytes[4] = tmp;
}

/**
 * @brief Reverse the order of bits in an integral number.
 *
 * @tparam T The integral type of the input and output.
 * @param number The number to have its bits reversed.
 * @return The resulting number with reversed bits.
 */
template<Integral T>
constexpr T reverse_bits(T number) noexcept {
    auto bytes = std::bit_cast<std::array<u8, sizeof(T)>>(number);
    reverse_byte_array(bytes);
    return std::bit_cast<T>(bytes);
}
