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
    concept Keyable = std::is_integral_v<T> || std::is_enum_v<T>;

    template<class T>
    concept KeyLike = requires(T key) {
        { T::count } -> std::convertible_to<Sz>;
        { key.to_index() } -> std::convertible_to<std::optional<Sz>>;
    };

    template<Keyable T, T LAST, T FIRST = static_cast<T>(0)>
        requires (FIRST <= LAST)
    class BasicKey {
    public:
        static Sz constexpr count = static_cast<Sz>(static_cast<i64>(LAST) - static_cast<i64>(FIRST) + 1);

        constexpr BasicKey(T value) noexcept
            : value(value) {}

        [[nodiscard]] std::optional<Sz> constexpr to_index() const noexcept {
            if (this->value >= FIRST && this->value <= LAST) {
                return static_cast<Sz>(static_cast<i64>(this->value) - static_cast<i64>(FIRST));
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

    template<Keyable T, T FIRST, T LAST>
        requires (FIRST <= LAST)
    struct KeyRange {
        static T constexpr first = FIRST;
        static T constexpr last = LAST;
    };

    template<Keyable T, KeyRangeLike<T>... Ranges>
        requires (sizeof...(Ranges) > 0)
    class SparseKey {
    public:
        static Sz constexpr count = (BasicKey<T, Ranges::last, Ranges::first>::count + ...);

        constexpr SparseKey(T value) noexcept
            : value(value) {}

        [[nodiscard]] std::optional<Sz> constexpr to_index() const noexcept {
            return to_index<BasicKey<T, Ranges::last, Ranges::first>...>(0);
        }

        T value;

    private:
        template<KeyLike Key, KeyLike... Keys>
        [[nodiscard]] std::optional<Sz> constexpr to_index(Sz accumulated_index) const noexcept {
            auto index = Key(this->value).to_index();
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
            : values(values...) {}

        [[nodiscard]] std::optional<Sz> constexpr to_index() const noexcept {
            return this->to_index_recursive<0>(1);
        }

        std::tuple<KeyTypes...> values;

    private:
        template<Sz KEY>
        [[nodiscard]] std::optional<Sz> constexpr to_index_recursive(Sz accumulated_count) const noexcept {
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

    template<KeyLike K, typename V>
    class StaticMap {
    public:
        constexpr StaticMap(std::initializer_list<std::pair<K, V>> list) {
            for (auto const& [key, value] : list) {
                auto index = key.to_index();
                if (!index) {
                    // Invalid constructor calls should be strictly forbidden
                    throw std::out_of_range("Invalid key value");
                }
                this->table.at(*index) = value;
            }
        }

        [[nodiscard]] constexpr V* operator[](K key) noexcept {
            return this->at(key);
        }

        [[nodiscard]] constexpr V const* operator[](K key) const noexcept {
            return this->at(key);
        }

    private:
        constexpr V* at(K key) noexcept {
            if (auto const index = key.to_index()) {
                if (auto& value = this->table[*index]) {
                    return &*value;
                }
            }

            return nullptr;
        }

        constexpr V const* at(K key) const noexcept {
            if (auto const index = key.to_index()) {
                if (auto const& value = this->table[*index]) {
                    return &*value;
                }
            }

            return nullptr;
        }

        std::array<std::optional<V>, K::count> table;
    };
};
