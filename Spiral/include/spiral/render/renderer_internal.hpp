#pragma once

#include "device.hpp"

#include <spiral/core/core.hpp>

namespace Spiral
{
	namespace renderer
	{
		void init(program_id engineProps, program_id clientProps);
		void terminate();
		void tick();

		device& get_device();
	}

	namespace shader_library
	{
		void clear();
	}
}
