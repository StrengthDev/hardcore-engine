#pragma once

#include "Spiral/Core/Core.hpp"

namespace Spiral
{
	class SPIRAL_API Renderer
	{
	public:
		virtual ~Renderer() = default;

		//virtual void setMainDevice() = 0;


		static Renderer* init(ECProperties engineProps, ECProperties clientProps);
	};
}