#pragma once

#include "Core.hpp"
#include "Spiral/Event/Event.hpp"

namespace Spiral
{
	class SPIRAL_API Layer
	{
	public:
		Layer();

		virtual ~Layer() = default;

		virtual void tick();

		virtual bool handleEvent(Event e);
	};
}