#include "pch.hpp"

#include "Event.hpp"

namespace Spiral
{
	Event Event::windowResize(uint16_t width, uint16_t height)
	{
		Event res = {
			EventType::WindowResize,
			EventCategory::Application,
			width,
			height
		};
		return res;
	}

	Event Event::windowClose()
	{
		Event res = {
			EventType::WindowClose,
			EventCategory::Application,
			0,
			0
		};
		return res;
	}
}