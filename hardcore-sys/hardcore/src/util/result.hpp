#pragma once

#include <variant>

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
