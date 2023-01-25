#pragma once

#include <functional>

#include <core/event.hpp>

#include <GLFW/glfw3.h>

namespace ENGINE_NAMESPACE
{
	namespace window
	{
		typedef std::function<void(Event&&)> event_callback_t;
		typedef std::function<void(int, int)> size_callback_t;

		/**TODO
		 * @brief 
		 * @param ec 
		 * @param sc 
		*/
		void init(event_callback_t&& ec, size_callback_t&& sc);

		/**TODO
		 * @brief 
		*/
		void terminate();

		/**TODO
		 * @brief 
		*/
		void tick();

		/**TODO
		 * @brief 
		 * @return 
		*/
		GLFWwindow* get_handle();
	}
}
