#pragma once

#include <functional>

#include "result.hpp"

namespace ENGINE_NAMESPACE
{
	namespace parallel //TODO: scrap results, use std::future/std::promise
	{
		template<typename Func>
		void execute(void* func_ptr)
		{
			Func& func = *static_cast<Func*>(func_ptr);
			func();
		} //TODO: use this helper function and pass it into submit function, eliminating the need to pass std::function 

		ENGINE_API void submit_immediate_task(std::function<void()> task);

		/**
		 * @brief Submits a task for immediate parallel execution.
		 * @tparam Type Return type of the task.
		 * @param task Function to be executed in parallel by another thread.
		 * @return result object which will contain the result of the function once it has been executed.
		*/
		template<typename Type>
		inline result<Type> immediate_task(func_t<Type> task)
		{
			result<Type> ret;
			void* sc = ret.shared_container; //compiler was complaining "sc" was undeclared when it was of type result<Type>::container*, idk
			submit_immediate_task([task, sc]()
				{
					result<Type>::set(*reinterpret_cast<result<Type>::container*>(sc), task());
					result<Type>::try_destroy_container(reinterpret_cast<result<Type>::container*>(sc));
				});
			return ret;
		}

		/**
		 * @brief Submits a task for immediate parallel execution.
		 * @param task Function to be executed in parallel by another thread.
		 * @return something todo
		*/
		template<>
		inline result<void> immediate_task<void>(func_t<void> task)
		{
			result<void> ret;
			void* sc = ret.shared_container;
			submit_immediate_task([task, sc]()
				{
					task();
					result<void>::set(*reinterpret_cast<result<void>::container*>(sc));
					result<void>::try_destroy_container(reinterpret_cast<result<void>::container*>(sc));
				});
			return ret;
		}

		ENGINE_API void submit_background_task(std::function<void()> task);

		template<typename Type>
		inline result<Type> background_task(func_t<Type> task)
		{
			result<Type> ret;
			submit_background_task([task, &ret]()
				{
					ret.set(task());
				});
			return ret;
		}

		template<>
		inline result<void> background_task<void>(func_t<void> task)
		{
			result<void> ret;
			submit_background_task([task, &ret]()
				{
					task();
					ret.set();
				});
			return ret;
		}
	}
}
