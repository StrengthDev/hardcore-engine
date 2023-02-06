#pragma once

#include <stdexcept>

#include "core.hpp"

namespace ENGINE_NAMESPACE
{
	namespace exception
	{
		//be sure to inherit from the correct class:
		//runtime_error for unpredictable errors such as failing to open a file
		//logic_error for preventable errors such as calling a function using invalid arguments

		class type_mismatch : std::logic_error
		{
		public:
			type_mismatch(const char* message) : std::logic_error(message) 
			{ }
			type_mismatch(const std::string& message) : std::logic_error(message)
			{ }
		};

		class closed_queue : std::logic_error
		{
		public:
			closed_queue(const char* message) : std::logic_error(message)
			{}
			closed_queue(const std::string& message) : std::logic_error(message)
			{}
		};
	}
}
