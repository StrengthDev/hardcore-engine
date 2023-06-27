#pragma once

#include <functional>
#include <future>

namespace ENGINE_NAMESPACE
{
	namespace parallel
	{
		template<typename Ret>
		using func_t = std::function<Ret()>;

		namespace internal
		{
			template<typename Type>
			inline void call_and_set(const func_t<Type>& func, std::promise<Type>& promise)
			{
				promise.set_value(func());
			}

			template<>
			inline void call_and_set<void>(const func_t<void>& func, std::promise<void>& promise)
			{
				func();
				promise.set_value();
			}

			template<typename Type>
			void execute(void* func_ptr, void* promise_ptr)
			{
				func_t<Type>* func = static_cast<func_t<Type>*>(func_ptr);
				std::promise<Type>* promise = static_cast<std::promise<Type>*>(promise_ptr);
				try
				{
					call_and_set<Type>(*func, *promise);
				}
				catch (const std::exception&)
				{
					//set_exception may also throw, but that should not be caught here
					//only exceptions thrown during the task execution should be caught
					promise->set_exception(std::current_exception());
				}
				delete func;
				delete promise;
			}

			ENGINE_API void submit_immediate_task(void(*aux)(void*, void*), void* func_ptr, void* promise_ptr);
			ENGINE_API void submit_background_task(void(*aux)(void*, void*), void* func_ptr, void* promise_ptr);
		}

		/**
		 * @brief Submits a task for immediate parallel execution.
		 * @tparam Type Return type of the task.
		 * @param task Function to be executed in parallel by another thread.
		 * @return std::future object which will contain the result of the function once it has been executed.
		*/
		template<typename Type>
		inline [[nodiscard("Task result should be checked")]] std::future<Type> immediate_async(func_t<Type> task)
		{
			std::promise<Type>* promise = new std::promise<Type>();
			func_t<Type>* func = new func_t<Type>(std::move(task));
			internal::submit_immediate_task(internal::execute<Type>, func, promise);
			return promise->get_future();
		}

		/**
		 * @brief Submits a task for parallel execution in the background.
		 * @tparam Type Return type of the task.
		 * @param task Function to be executed in parallel by another thread.
		 * @return std::future object which will contain the result of the function once it has been executed.
		*/
		template<typename Type>
		inline [[nodiscard("Task result should be checked")]]  std::future<Type> background_async(func_t<Type> task)
		{
			std::promise<Type>* promise = new std::promise<Type>();
			func_t<Type>* func = new func_t<Type>(std::move(task));
			internal::submit_background_task(internal::execute<Type>, func, promise);
			return promise->get_future();
		}
	}
}
