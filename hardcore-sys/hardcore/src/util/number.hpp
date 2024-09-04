#pragma once

#include <cstdint>
#include <cstddef>

/**
 * @brief Transform a number to be in the Kilo order of magnitude.
 *
 * @example KILO(1) == 1,000
 *
 * @param value The number to be transformed.
 */
#define KILO(value) ((value) * 1000)

/**
 * @brief Transform a number to be in the Mega order of magnitude.
 *
 * @example MEGA(1) == 1,000,000
 *
 * @param value The number to be transformed.
 */
#define MEGA(value) (KILO(value) * 1000)

/**
 * @brief Transform a number to be in the Kilo order of magnitude.
 *
 * @example GIGA(1) == 1,000,000,000
 *
 * @param value The number to be transformed.
 */
#define GIGA(value) (MEGA(value) * 1000)


/**
 * @brief Transform a number to be in the Kibi order of magnitude.
 *
 * @example KIBI(1) == 1,024
 *
 * @param value The number to be transformed.
 */
#define KIBI(value) ((value) << 10)

/**
 * @brief Transform a number to be in the Mibi order of magnitude.
 *
 * @example MEBI(1) == 1,048,576
 *
 * @param value The number to be transformed.
 */
#define MEBI(value) (KIBI(value) << 10)

/**
 * @brief Transform a number to be in the Gibi order of magnitude.
 *
 * @example GIBI(1) == 1,073,741,824
 *
 * @param value The number to be transformed.
 */
#define GIBI(value) (MEBI(value) << 10)

typedef std::uint8_t u8; //!< Short alias for a 8 bit unsigned integer.
typedef std::uint16_t u16; //!< Short alias for a 16 bit unsigned integer.
typedef std::uint32_t u32; //!< Short alias for a 32 bit unsigned integer.
typedef std::uint64_t u64; //!< Short alias for a 64 bit unsigned integer.

typedef std::int8_t i8; //!< Short alias for a 8 bit integer.
typedef std::int16_t i16; //!< Short alias for a 16 bit integer.
typedef std::int32_t i32; //!< Short alias for a 32 bit integer.
typedef std::int64_t i64; //!< Short alias for a 64 bit integer.

typedef std::size_t Sz; //!< Short alias for the standard size type. (`std::size_t`)
