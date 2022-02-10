#pragma once

#include <thread>
#include <mutex>

#include <spiral/core/core.hpp>

namespace Spiral
{
	namespace parallel
	{
		template<typename Type>
		class result;

		template<typename Ret>
		using func_t = std::function<Ret()>;

		template<typename Type>
		result<Type> immediate_task(func_t<Type>);

		template<typename Type>
		result<Type> background_task(Type(*)());

		template<typename Type>
		class result
		{
		public:
			result()
			{
				value = (Type*)ex_malloc(sizeof(Type));
			}

			result(result<Type>&& other) noexcept : ready(std::exchange(other.ready, false)), value(std::exchange(other.value, nullptr))
			{ }
			
			result(const result<Type>&) = delete;

			~result()
			{
				free(value);
			}

			inline bool is_ready() //TODO: change to use atomic flag
			{
				std::lock_guard<std::mutex> lock(access);
				return ready;
			}

			inline Type get()
			{
				while (!is_ready())
				{
					std::this_thread::yield(); //TODO: change to use condition variable wait
				}
				return *value;
			}

		private:

			inline void set(Type&& v)
			{
				std::lock_guard<std::mutex> lock(access);
				(*value) = std::move(v);
				ready = true;
			}

			friend result<Type> immediate_task<Type>(func_t<Type>);
			friend result<Type> background_task<Type>(Type(*)());

			std::mutex access;
			bool ready = false;
			Type* value = nullptr;
			//bool valid = false;
			//std::exception error;

		};

		template<>
		class result<void>
		{
		public:
			result() = default;

			result(result<void>&& other) noexcept : ready(std::exchange(other.ready, false))
			{ }

			result(const result<void>&) = delete;

			inline bool is_ready()
			{
				std::lock_guard<std::mutex> lock(access);
				return ready;
			}

			inline void get()
			{
				while (!is_ready())
				{
					std::this_thread::yield();
				}
			}

		private:

			inline void set()
			{
				std::lock_guard<std::mutex> lock(access);
				ready = true;
			}

			friend result<void> immediate_task<void>(func_t<void>);
			friend result<void> background_task<void>(void(*)());

			std::mutex access;
			bool ready = false;
			//bool valid = false;
			//std::exception error;

		};
	}
}
