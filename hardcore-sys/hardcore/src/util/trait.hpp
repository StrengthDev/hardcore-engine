#pragma once

#include <vector>

/**
 * @brief Trait type to check if a type is a standard library vector.
 * 
 * @tparam T The type to check.
 */
template<typename T>
struct is_std_vector final : std::false_type {
};

template<typename... T>
struct is_std_vector<std::vector<T...>> final : std::true_type {
};
