#pragma once

#include <mutex>
#include <condition_variable>
#include <new>
#include <exception>

#include <core/core.hpp>
#include <core/exception.hpp>

namespace ENGINE_NAMESPACE
{
	namespace parallel
	{
		template<typename T>
		class concurrent_queue
		{
		public:

			typedef std::size_t index_t;

			concurrent_queue()
			{
				items = t_malloc<T>(item_capacity);
			}

			concurrent_queue(concurrent_queue&& other) noexcept
				: item_capacity(std::exchange(other.item_capacity, 0)), items(std::exchange(other.items, nullptr)), n_items(std::exchange(other.n_items, 0)), first_item_idx(std::exchange(other.first_item_idx, 0)), closed(std::exchange(other.closed, false))
			{}

			concurrent_queue(const concurrent_queue&) = delete;

			concurrent_queue& operator=(concurrent_queue&& other) noexcept
			{
				item_capacity = std::exchange(other.item_capacity, 0);
				items = std::exchange(other.items, nullptr);
				n_items = std::exchange(other.n_items, 0);
				first_item_idx = std::exchange(other.first_item_idx, 0);
				closed = std::exchange(other.closed, true);

				new (&access) std::mutex();
				new (&signal) std::condition_variable();
				return *this;
			}

			concurrent_queue& operator=(const concurrent_queue&) = delete;

			~concurrent_queue()
			{
				std::free(items);
			}

			inline void push(const T& item)
			{
				std::lock_guard<std::mutex> lock(access);
				if (closed)
				{
					throw exception::closed_queue("Queue has already been closed");
				}
				if (n_items + 1 > item_capacity)
				{
					index_t new_capacity = item_capacity * 2;
					T* t = t_malloc<T>(new_capacity);

					index_t n_copies = item_capacity - first_item_idx;
					std::memcpy(t, &items[first_item_idx], n_copies * sizeof(T));
					std::memcpy(&t[n_copies], items, first_item_idx * sizeof(T));
					std::free(items);
					items = t;
					item_capacity = new_capacity;
				}
				new (&items[(n_items + first_item_idx) % item_capacity]) T(item);
				n_items++;
				signal.notify_all();
			}

			inline void push(T&& item)
			{
				std::lock_guard<std::mutex> lock(access);
				if (closed)
				{
					throw exception::closed_queue("Queue has already been closed");
				}
				if (n_items + 1 > item_capacity)
				{
					index_t new_capacity = item_capacity * 2;
					T* t = t_malloc<T>(new_capacity);

					index_t n_copies = item_capacity - first_item_idx;
					std::memcpy(t, &items[first_item_idx], n_copies * sizeof(T));
					std::memcpy(&t[n_copies], items, first_item_idx * sizeof(T));
					std::free(items);
					items = t;
					item_capacity = new_capacity;
				}
				new (&items[(n_items + first_item_idx) % item_capacity]) T(std::move(item));
				n_items++;
				signal.notify_all();
			}

			inline T pop()
			{
				std::unique_lock<std::mutex> lock(access);
				while (!n_items)
				{
					if (closed)
					{
						throw exception::closed_queue("Queue has already been closed");
					}
					signal.wait(lock);
				}
				T item(std::move(items[first_item_idx]));
				first_item_idx = (first_item_idx + 1) % item_capacity;
				n_items--;
				if (!n_items)
				{
					std::unique_lock<std::mutex> lock(empty_mutex);
					empty_signal.notify_all();
				}
				return item;
			}

			inline bool try_pop(T& out_item)
			{
				if (access.try_lock())
				{
					if (!n_items)
					{
						access.unlock();
						return false;
					}
					out_item = std::move(items[first_item_idx]);
					first_item_idx = (first_item_idx + 1) % item_capacity;
					n_items--;
					if (!n_items)
					{
						std::unique_lock<std::mutex> lock(empty_mutex);
						empty_signal.notify_all();
					}
					access.unlock();
					return true;
				}
				return false;
			}

			inline index_t size()
			{
				std::lock_guard<std::mutex> lock(access);
				return n_items;
			}

			//Use only if accesses to the queue are manually managed
			inline index_t unsafe_size()
			{
				return n_items;
			}

			inline void close()
			{
				std::lock_guard<std::mutex> lock(access);
				closed = true;
				signal.notify_all();
			}

			inline void wait_until_empty()
			{
				std::unique_lock<std::mutex> lock(empty_mutex);
				empty_signal.wait(lock);
			}

		private:
			std::condition_variable signal;
			std::mutex access;

			std::condition_variable empty_signal;
			std::mutex empty_mutex;

			index_t item_capacity = BIT(4);
			T* items = nullptr;

			index_t n_items = 0;
			index_t first_item_idx = 0;

			bool closed = false;
		};
	}
}
