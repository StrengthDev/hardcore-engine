
#pragma once

#include <shared_mutex>
#include <type_traits>
#include <utility>

template<typename T>
class ReadGuard {
public:
    operator T const&() const {
        return this->value;
    }

    T const* operator->() const noexcept {
        return &this->value;
    }

private:
    ReadGuard(std::shared_mutex& mutex, T const& value) : lock(mutex), value(value) {}

    std::shared_lock<std::shared_mutex> lock;
    T const& value;

    template<typename TT>
    friend class Mutex;
};

template<typename T>
class WriteGuard {
public:
    operator T&() const {
        return this->value;
    }

    T* operator->() const noexcept {
        return &this->value;
    }

private:
    WriteGuard(std::shared_mutex& mutex, T& value) : lock(mutex), value(value) {}

    std::unique_lock<std::shared_mutex> lock;
    T& value;

    template<typename TT>
    friend class Mutex;
};

template<typename T>
class Mutex {
public:
    Mutex(Mutex&& other) noexcept requires (std::is_move_constructible_v<T>) : value(std::move(other.value)) {}

    Mutex& operator=(Mutex&& other) noexcept requires (std::is_move_assignable_v<T>) {
        this->value = std::move(other.value);
        return *this;
    }

    template<typename... Args>
    Mutex(Args&&... args) : value(std::forward<Args>(args)...) {}

    ReadGuard<T> read() const {
        return ReadGuard(this->mutex, this->value);
    }

    WriteGuard<T> write() {
        return WriteGuard(this->mutex, this->value);
    }

private:
    mutable std::shared_mutex mutex;
    T value;
};
