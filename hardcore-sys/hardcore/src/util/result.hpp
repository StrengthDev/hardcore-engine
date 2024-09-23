#pragma once

#include <variant>

/**
 * @brief A thin success wrapper around an success value.
 *
 * This class may be implicitly converted to a `Result` object.
 *
 * @tparam T The type of the inner success value.
 */
template<typename T>
class Ok {
public:
    /**
     * @brief Create an success from an success value.
     *
     * @param value The success value.
     */
    constexpr explicit Ok(T &&value) : value(std::move(value)) {}

    /**
     * @brief Create an success from an success value.
     *
     * @param value The success value.
     */
    constexpr explicit Ok(const T &value) : value(value) {}

    Ok() = delete;

    Ok(const Ok &) = delete;

    Ok &operator=(const Ok &) = delete;

    Ok(Ok &&) = default;

    Ok &operator=(Ok &&) = default;

    /**
     * @brief Retrieve the inner success value.
     *
     * @return The inner success value.
     */
    constexpr T &&operator*() && noexcept {
        return std::move(this->value);
    }

private:
    T value; //!< The inner success value.
};

/**
 * @brief A thin error wrapper around an error value.
 *
 * This class may be implicitly converted to a `Result` object.
 *
 * @tparam T The type of the inner error value.
 */
template<typename T>
class Err {
public:
    /**
     * @brief Create an error from an error value.
     *
     * @param value The error value.
     */
    constexpr explicit Err(T &&value) : value(std::move(value)) {}

    /**
     * @brief Create an error from an error value.
     *
     * @param value The error value.
     */
    constexpr explicit Err(const T &value) : value(value) {}

    Err() = delete;

    Err(const Err &) = delete;

    Err &operator=(const Err &) = delete;

    Err(Err &&) = default;

    Err &operator=(Err &&) = default;

    /**
     * @brief Retrieve the inner error value.
     *
     * @return The inner error value.
     */
    constexpr T &&operator*() && noexcept {
        return std::move(this->value);
    }

private:
    T value; //!< The inner error value.
};

/**
 * @brief A class representing the result of some operation, typically returned by functions.
 *
 * This class loosely mimics Rust's Result enum, to facilitate error handling.
 *
 * @tparam S The type of the value returned on success.
 * @tparam E The error type, returned by unsuccessful operations.
 */
template<typename S, typename E>
class Result {
public:
    // No default constructor.
    Result() = delete;

    /**
     * @brief Create a new successful `Result` from an `Ok` rvalue.
     *
     * @param ok The value from which the `Result` is created.
     */
    constexpr Result(Ok<S> &&ok) : // NOLINT intentionally implicit
            Result(std::variant<S, E>(std::in_place_index<0>, *std::move(ok))) {}

    /**
     * @brief Create a new unsuccessful `Result` from an `Error` rvalue.
     *
     * @param err The value from which the `Result` is created.
     */
    constexpr Result(Err<E> &&err) : // NOLINT intentionally implicit
            Result(std::variant<S, E>(std::in_place_index<1>, *std::move(err))) {}

    /**
     * @brief Get the success value.
     *
     * @throw std::bad_variant_access If this `Result` does not represent a success.
     *
     * @return The success value, as a const reference.
     */
    constexpr const S &ok() const &{
        return std::get<0>(this->value);
    }

    /**
     * @brief Get the success value.
     *
     * @throw std::bad_variant_access If this `Result` does not represent a success.
     *
     * @return The success value, as an rvalue.
     */
    constexpr S &&ok() &&{
        return std::get<0>(std::move(this->value));
    }

    /**
     * @brief Get the error value.
     *
     * @throw std::bad_variant_access If this `Result` does not represent an error.
     *
     * @return The error value, as a const reference.
     */
    constexpr const E &err() const &{
        return std::get<1>(this->value);
    }

    /**
     * @brief Get the error value.
     *
     * @throw std::bad_variant_access If this `Result` does not represent an error.
     *
     * @return The error value, as an rvalue.
     */
    constexpr E &&err() &&{
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
    explicit Result(std::variant<S, E> &&value) : value(std::move(value)) {}

    // std::variant does all the heavy lifting for destruction, assignments, etc..
    std::variant<S, E> value; //!< The inner result value.
};
