#pragma once

#include "core.hpp"

namespace ENGINE_NAMESPACE
{
	namespace window
	{
		/**
		 * @brief Retrives the width and height of the window.
		 * This function assigns the width and height of the window, in pixels, to the arguments.
		 * @param[out] out_width Where to store the window width value.
		 * @param[out] out_height Where to store the window height value.
		*/
		ENGINE_API void get_dimensions(int* out_width, int* out_height);

		/**TODO
		 * @brief 
		 * @param title 
		*/
		ENGINE_API void set_title(const char* title);

		/**TODO
		 * @brief 
		 * @param files 
		 * @param num 
		*/
		ENGINE_API void set_icon(const char** files, unsigned int num);
	}
}
