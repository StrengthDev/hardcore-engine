#pragma once

#include <unordered_map>
#include <utility>

#include "number.hpp"
#include "flow.hpp"

/**
 * @brief An associative container similar to a map.
 *
 * Unlike a map, when a new element is inserted, its id (or key) is generated and returned by the insertion function.
 *
 * @tparam T The type of each element contained by the bank.
 */
template<typename T>
class Bank {
public:
    using ID = u64; //!< The type used to identify values held within the bank.

    /**
     * @brief Insert a new element into the bank.
     *
     * @param value The element to be inserted.
     * @return The id of the newly inserted element.
     */
    [[nodiscard]] ID insert(T &&value) {
        // Unsure of this is the best way to generate IDs, but in practice there should no collisions at least.
        ID id = this->seed++;
        this->map.insert({id, std::move(value)});
        return id;
    }

    /**
     * @brief Remove an element from the bank.
     *
     * @param id The id of the element to be removed.
     * @return The removed element.
     */
    T erase(ID id) {
        auto i = this->map.find(id);
        HC_ASSERT(i != this->map.end(), "Element must exist");
        auto v = std::move(i->second);
        this->map.erase(id);
        return std::move(v);
    }

    /**
     * @brief Clears the contents of the bank.
     */
    void clear() {
        this->map.clear();
        this->seed = 0;
    }

    /**
     * @brief Returns the number of element within the bank.
     *
     * @return The number of elements.
     */
    [[nodiscard]] Sz size() const noexcept {
        return this->map.size();
    }

    /**
     * @brief Checks whether the bank is empty.
     *
     * @return `true` if the bank is empty, `false` otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return this->map.empty();
    }

    /**
     * @brief Check if an element with a specific id exists within the bank.
     *
     * @param id The element ID to verify.
     * @return `true` if the element exists, `false` otherwise.
     */
    [[nodiscard]] bool contains(ID id) const {
        return this->map.contains(id);
    }

    /**
     * @brief Access specific element.
     *
     * @param id The id of the element to be accessed.
     * @return A reference to the element.
     */
    [[nodiscard]] T &operator[](ID id) noexcept {
        HC_ASSERT(this->map.contains(id), "Value matching the id must already exist");
        return this->map.at(id);
    }

    /**
     * @brief Access specific element.
     *
     * @param id The id of the element to be accessed.
     * @return A reference to the element.
     */
    [[nodiscard]] const T &operator[](ID id) const noexcept {
        HC_ASSERT(this->map.contains(id), "Value matching the id must already exist");
        return this->map.at(id);
    }

    /**
     * @brief Returns an iterator to the beginning of the bank.
     *
     * @return The iterator.
     */
    [[nodiscard]] auto begin() noexcept {
        return this->map.begin();
    }

    /**
     * @brief Returns an iterator to the end of the bank.
     *
     * @return The iterator.
     */
    [[nodiscard]] auto end() noexcept {
        return this->map.end();
    }

    /**
     * @brief Returns a const iterator to the beginning of the bank.
     *
     * @return The iterator.
     */
    [[nodiscard]] auto begin() const noexcept {
        return this->map.begin();
    }

    /**
     * @brief Returns a const iterator to the end of the bank.
     *
     * @return The iterator.
     */
    [[nodiscard]] auto end() const noexcept {
        return this->map.end();
    }

private:
    std::unordered_map<ID, T> map; //!< The inner collection of values.
    ID seed = 0; //!< The seed used to generate each new value id.
};
