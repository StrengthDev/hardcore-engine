#pragma once

#include <mutex>
#include <condition_variable>
#include <new>
#include <functional>
#include <exception>

#include <core/core.hpp>

namespace ENGINE_NAMESPACE
{
	namespace parallel
	{
		void launch_threads();
		void terminate_threads();

		class closed_queue : public std::runtime_error
		{
		public:
			closed_queue() : std::runtime_error("closed queue")
			{ }
		};

		class task_queue
		{
		public:

			typedef std::function<void()> task_t;
			typedef std::size_t index_t;

			task_queue()
			{
				tasks = t_malloc<task_t>(task_capacity);
			}

			task_queue(task_queue&& other) noexcept 
				: task_capacity(std::exchange(other.task_capacity, 0)), tasks(std::exchange(other.tasks, nullptr)), n_tasks(std::exchange(other.n_tasks, 0)), first_task_idx(std::exchange(other.first_task_idx, 0)), closed(std::exchange(other.closed, false))
			{ }

			task_queue(const task_queue&) = delete;

			task_queue& operator=(task_queue&& other) noexcept
			{
				task_capacity = std::exchange(other.task_capacity, 0);
				tasks = std::exchange(other.tasks, nullptr);
				n_tasks = std::exchange(other.n_tasks, 0);
				first_task_idx = std::exchange(other.first_task_idx, 0);
				closed = std::exchange(other.closed, true);

				new (&access) std::mutex();
				new (&signal) std::condition_variable();
				return *this;
			}

			task_queue& operator=(const task_queue&) = delete;

			~task_queue()
			{
				std::free(tasks);
			}

			inline void push(const task_t& task)
			{
				std::lock_guard<std::mutex> lock(access);
				if (closed)
				{
					throw closed_queue();
				}
				if (n_tasks + 1 > task_capacity)
				{
					index_t new_capacity = task_capacity * 2;
					task_t* t = t_malloc<task_t>(new_capacity);

					index_t n_copies = task_capacity - first_task_idx;
					std::memcpy(t, &tasks[first_task_idx], n_copies * sizeof(task_t));
					std::memcpy(&t[n_copies], tasks, first_task_idx * sizeof(task_t));
					std::free(tasks);
					tasks = t;
					task_capacity = new_capacity;
				}
				new (&tasks[(n_tasks + first_task_idx) % task_capacity]) task_t(task);
				n_tasks++;
				signal.notify_all();
			}

			inline void push(task_t&& task)
			{
				std::lock_guard<std::mutex> lock(access);
				if (closed)
				{
					throw closed_queue();
				}
				if (n_tasks + 1 > task_capacity)
				{
					index_t new_capacity = task_capacity * 2;
					task_t* t = t_malloc<task_t>(new_capacity);

					index_t n_copies = task_capacity - first_task_idx;
					std::memcpy(t, &tasks[first_task_idx], n_copies * sizeof(task_t));
					std::memcpy(&t[n_copies], tasks, first_task_idx * sizeof(task_t));
					std::free(tasks);
					tasks = t;
					task_capacity = new_capacity;
				}
				new (&tasks[(n_tasks + first_task_idx) % task_capacity]) task_t(std::move(task));
				n_tasks++;
				signal.notify_all();
			}

			inline task_t pop()
			{
				std::unique_lock<std::mutex> lock(access);
				while (!n_tasks)
				{
					if (closed)
					{
						throw closed_queue();
					}
					signal.wait(lock);
				}
				task_t task(std::move(tasks[first_task_idx]));
				first_task_idx = (first_task_idx + 1) % task_capacity;
				n_tasks--;
				return task;
			}

			inline bool try_pop(task_t& out_task)
			{
				if (access.try_lock())
				{
					if (!n_tasks)
					{
						access.unlock();
						return false;
					}
					out_task = std::move(tasks[first_task_idx]);
					first_task_idx = (first_task_idx + 1) % task_capacity;
					n_tasks--;

					access.unlock();
					return true;
				}
				return false;
			}

			inline index_t size()
			{
				std::lock_guard<std::mutex> lock(access);
				return n_tasks;
			}

			//Use only if accesses to the queue are manually managed
			inline index_t unsafe_size()
			{
				return n_tasks;
			}

			inline void close()
			{
				std::lock_guard<std::mutex> lock(access);
				closed = true;
				signal.notify_all();
			}


		private:
			std::condition_variable signal;
			std::mutex access;

			index_t task_capacity = BIT(4);
			task_t* tasks = nullptr;

			index_t n_tasks = 0;
			index_t first_task_idx = 0;

			bool closed = false;
		};
	}
}
