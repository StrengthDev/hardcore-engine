#pragma once

#include <thread>
#include <mutex>
#include <type_traits>

#include <hardcore/core/core.hpp>

namespace ENGINE_NAMESPACE
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
		result<Type> background_task(func_t<Type>);

		template<typename Type>
		class result
		{
		public:
			result(result<Type>&& other) noexcept : ready(std::exchange(other.ready, false)), shared_container(std::exchange(other.shared_container, nullptr))
			{
				shared_container->owner = this;
			}

			result<Type>& operator=(result<Type>&& other) noexcept
			{
				//TODO
				return *this;
			}
			
			result(const result<Type>&) = delete;
			result<Type>& operator=(const result<Type>& other) = delete;

			~result()
			{
				if (shared_container)
				{
					try_destroy_container(shared_container);
				}
			}

			inline bool is_ready() //TODO: change to use atomic flag
			{
				std::lock_guard<std::mutex> lock(shared_container->access);
				return ready;
			}

			inline Type get()
			{
				while (!is_ready())
				{
					std::this_thread::yield(); //TODO: change to use condition variable wait
				}
				return (*shared_container).value;
			}

		private:
			result()
			{
				shared_container = t_calloc<container>(1);
				if constexpr (std::is_default_constructible<Type>::value && !std::is_pointer<Type>::value)
				{
					new (&(shared_container->value)) Type();
				}
				new (&(shared_container->access)) std::mutex();
				shared_container->owner = this;
			}

			inline void set(Type&& v)
			{
				(*shared_container).value = std::move(v);
				ready = true;
			}

			friend result<Type> immediate_task<Type>(func_t<Type>);
			friend result<Type> background_task<Type>(func_t<Type>);

			struct container
			{
				Type value;
				std::mutex access;
				bool single_ref;
				result<Type>* owner;
			};

			inline static void set(result<Type>::container& shared, Type&& v)
			{
				shared.access.lock();
				if (!shared.single_ref)
				{
					shared.owner->set(std::move(v));
				}
				shared.access.unlock();
			}

			inline static void try_destroy_container(result<Type>::container* shared)
			{
				shared->access.lock();
				if (shared->single_ref)
				{
					shared->access.unlock();
					shared->~container();
					std::free(shared);
				}
				else
				{
					shared->single_ref = true;
					shared->access.unlock();
				}
			}

			bool ready = false;
			container* shared_container = nullptr;
			//bool valid = false;
			//std::exception error;

		};

		template<>
		class result<void>
		{
		public:
			result(result<void>&& other) noexcept : ready(std::exchange(other.ready, false)), shared_container(std::exchange(other.shared_container, nullptr))
			{
				shared_container->owner = this;
			}

			result<void>& operator=(result<void>&& other) noexcept
			{
				//TODO
				return *this;
			}

			result(const result<void>&) = delete;
			result<void>& operator=(const result<void>& other) = delete;

			~result()
			{
				if (shared_container)
				{
					try_destroy_container(shared_container);
				}
			}

			inline bool is_ready()
			{
				std::lock_guard<std::mutex> lock(shared_container->access);
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
			result()
			{
				shared_container = t_calloc<container>(1);
				new (&(shared_container->access)) std::mutex();
				shared_container->owner = this;
			}

			inline void set()
			{
				ready = true;
			}

			friend result<void> immediate_task<void>(func_t<void>);
			friend result<void> background_task<void>(func_t<void>);

			struct container
			{
				std::mutex access;
				bool single_ref;
				result<void>* owner;
			};

			inline static void set(result<void>::container& shared)
			{
				shared.access.lock();
				if (!shared.single_ref)
				{
					shared.owner->set();
				}
				shared.access.unlock();
			}

			inline static void try_destroy_container(result<void>::container* shared)
			{
				shared->access.lock();
				if (shared->single_ref)
				{
					shared->access.unlock();
					shared->~container();
					std::free(shared);
				}
				else
				{
					shared->single_ref = true;
					shared->access.unlock();
				}
			}

			bool ready = false;
			container* shared_container = nullptr;
			//bool valid = false;
			//std::exception error;

		};
	}
}
