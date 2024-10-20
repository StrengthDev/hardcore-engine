#pragma once

#include <utility>

#include "flow.hpp"

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

	Uncopyable(const T &value) : value(value) { ; } // NOLINT intentionally implicit

	Uncopyable(const Uncopyable &) = delete;

	Uncopyable &operator=(const Uncopyable &) = delete;

	Uncopyable(Uncopyable &&other) noexcept: value(std::exchange(other.value, DEFAULT)) {
	}

	Uncopyable &operator=(Uncopyable &&other) noexcept {
		this->value = std::exchange(other.value, DEFAULT);
		return *this;
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
	ExternalHandle(const T &value) : value(value) { ; } // NOLINT intentionally implicit

	~ExternalHandle() {
		HC_ASSERT(this->value == DEFAULT, "Inner value must be externally cleaned up");
	}

	ExternalHandle(const ExternalHandle &) = delete;

	ExternalHandle &operator=(const ExternalHandle &) = delete;

	ExternalHandle(ExternalHandle &&other) noexcept: value(std::exchange(other.value, DEFAULT)) {
	}

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
