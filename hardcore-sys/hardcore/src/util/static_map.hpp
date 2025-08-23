#pragma once

#include "number.hpp"

#include <array>
#include <concepts>
#include <initializer_list>
#include <optional>
#include <tuple>
#include <type_traits>

namespace hc {
    template<class T>
    concept KeyLike = requires(T key) {
        { T::count } -> std::convertible_to<Sz>;
        { key.to_index() } -> std::convertible_to<std::optional<Sz>>;
    };

    template<typename T, T LAST, T FIRST = static_cast<T>(0)>
        requires ((std::is_integral_v<T> || std::is_enum_v<T>) && FIRST <= LAST)
    class BasicKey {
    public:
        static Sz constexpr count = static_cast<Sz>(LAST - FIRST + 1);

        constexpr BasicKey(T value) noexcept
            : value(value) {
        }

        std::optional<Sz> constexpr to_index() const noexcept {
            if (this->value >= FIRST && this->value <= LAST) {
                return this->value - FIRST;
            }

            return std::nullopt;
        }

        T value;
    };

    template<class R, typename T>
    concept KeyRangeLike = requires(R range) {
        { R::first } -> std::convertible_to<T>;
        { R::last } -> std::convertible_to<T>;
    };

    template<typename KeyType, KeyType FIRST, KeyType LAST>
        requires ((std::is_integral_v<KeyType> || std::is_enum_v<KeyType>) && FIRST <= LAST)
    struct KeyRange {
        static KeyType constexpr first = FIRST;
        static KeyType constexpr last = LAST;
    };

    template<typename T, typename... Ranges>
        requires (KeyRangeLike<Ranges, T> && ...) && (0 < sizeof...(Ranges))
    class SparseKey {
    public:
        static Sz constexpr count = (BasicKey<T, Ranges::last, Ranges::first>::count + ...);

        constexpr SparseKey(T value) noexcept
            : value(value) {
        }

        std::optional<Sz> constexpr to_index() const noexcept {
            return to_index<BasicKey<T, Ranges::last, Ranges::first>...>(0);
        }

        T value;

    private:
        template<KeyLike Key, KeyLike... Keys>
        std::optional<Sz> constexpr to_index(Sz accumulated_index) const noexcept {
            auto index = Key(value).to_index();
            index = index.transform([accumulated_index](Sz value) { return accumulated_index + value; });

            if constexpr (0 < sizeof...(Keys)) {
                if (index) {
                    return index;
                }

                return to_index<Keys...>(accumulated_index + Key::count);
            } else {
                return index;
            }
        }
    };

    template<KeyLike... KeyTypes>
    class KeyUnion {
    public:
        static Sz constexpr count = (KeyTypes::count * ...);

        constexpr KeyUnion(KeyTypes... values) noexcept
            : values(values...) {
        }

        std::optional<Sz> constexpr to_index() const noexcept {
            return this->to_index_recursive<0>(1);
        }

        std::tuple<KeyTypes...> values;

    private:
        template<Sz KEY>
        std::optional<Sz> constexpr to_index_recursive(Sz accumulated_count) const noexcept {
            auto key = std::get<KEY>(this->values);
            using KeyType = decltype(key);

            Sz accumulated_index = 0;
            if constexpr (KEY + 1 < sizeof...(KeyTypes)) {
                auto accumulated_index_opt = this->to_index_recursive<KEY + 1>(accumulated_count * KeyType::count);
                if (!accumulated_index_opt) {
                    return std::nullopt;
                }

                accumulated_index = *accumulated_index_opt;
            }

            if (auto const index = key.to_index()) {
                return *index * accumulated_count + accumulated_index;
            }

            return std::nullopt;
        }
    };

    template<KeyLike KeyType, typename T>
    class StaticMap {
    public:
        constexpr StaticMap(std::initializer_list<std::pair<KeyType, T>> list) {
            for (auto const& [key, value] : list) {
                auto index = key.to_index();
                if (!index) {
                    // Invalid constructor calls should be strictly forbidden
                    throw std::out_of_range("Invalid key value");
                }
                this->table.at(*index) = value;
            }
        }

        constexpr T* operator[](KeyType key) noexcept {
            return this->at(key);
        }

        constexpr T const* operator[](KeyType key) const noexcept {
            return this->at(key);
        }

    private:
        constexpr T* at(KeyType key) noexcept {
            if (auto const index = key.to_index()) {
                if (auto& value = this->table[*index]) {
                    return &*value;
                }
            }

            return nullptr;
        }

        constexpr T const* at(KeyType key) const noexcept {
            if (auto const index = key.to_index()) {
                if (auto const& value = this->table[*index]) {
                    return &*value;
                }
            }

            return nullptr;
        }

        std::array<std::optional<T>, KeyType::count> table;
    };
};
