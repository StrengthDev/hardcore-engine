#pragma once

#include <array>

#include "number.hpp"
#include "flow.hpp"

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
    static_assert(InstantiatedVarFalse<decltype(N), N>::value, "Impossible number byte size");
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
