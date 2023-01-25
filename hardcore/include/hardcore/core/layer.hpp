#pragma once

#include "core.hpp"
#include "event.hpp"

namespace ENGINE_NAMESPACE
{
	class ENGINE_API Layer
	{
	public:
		Layer();

		virtual ~Layer() = default;

		virtual void tick();

		virtual bool handleEvent(const Event &e);
	};
}