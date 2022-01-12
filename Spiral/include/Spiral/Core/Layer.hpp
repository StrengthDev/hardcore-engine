#pragma once

#include "core.hpp"
#include "event.hpp"

namespace Spiral
{
	class SPIRAL_API Layer
	{
	public:
		Layer();

		virtual ~Layer() = default;

		virtual void tick();

		virtual bool handleEvent(const Event &e);
	};
}