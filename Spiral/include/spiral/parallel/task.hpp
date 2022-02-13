#pragma once

#include <functional>

#include "result.hpp"

namespace Spiral
{
	namespace parallel
	{
		SPIRAL_API void submit_immediate_task(std::function<void()> task);

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
			submit_immediate_task([task, &ret]()
				{
					ret.set(task()); //TODO: pointer sync with result
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
			submit_immediate_task([task]()
				{
					task();
					//ret.set();
				});
			return ret;
		}

		SPIRAL_API void submit_background_task(std::function<void()> task);

		template<typename Type>
		inline result<Type> background_task(Type(*task)())
		{
			result<Type> ret;
			submit_background_task([task, &ret]()
				{
					ret.set(task());
				});
			return ret;
		}

		template<>
		inline result<void> background_task<void>(void(*task)())
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
