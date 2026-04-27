
#pragma once

#include "mutex.hpp"
#include "number.hpp"

#include <queue>

template<typename T>
class ConcurrentQueue {
public:
    ConcurrentQueue() = default;

    ConcurrentQueue(ConcurrentQueue&&) = default;
    ConcurrentQueue(ConcurrentQueue const&) = delete;

    ConcurrentQueue& operator=(ConcurrentQueue&&) = default;
    ConcurrentQueue& operator=(ConcurrentQueue const&) = delete;

    void push(T&& value) noexcept {
        auto guard = this->queue.write();
        guard->emplace(std::move(value));
    }

    void push(T const& value) noexcept {
        auto guard = this->queue.write();
        guard->emplace(value);
    }

    T pop() noexcept {
        auto guard = this->queue.write();
        T value = std::move(guard->front());
        guard->pop();
        return std::move(value);
    }

    std::queue<T> drain() noexcept {
        auto guard = this->queue.write();
        if (guard->empty()) {
            return std::queue<T>();
        }

        std::queue<T> ret;

        std::swap(ret, static_cast<std::queue<T>&>(guard));

        return std::move(ret);
    }

    [[nodiscard]] Sz size() const noexcept {
        return this->queue.read()->size();
    }

    [[nodiscard]] bool empty() const noexcept {
        return this->queue.read()->empty();
    }

private:
    Mutex<std::queue<T>> queue;
};
