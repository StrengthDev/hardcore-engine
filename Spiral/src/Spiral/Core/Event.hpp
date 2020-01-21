#pragma once

#include "Core.hpp"

#include <cstdint>

namespace Spiral
{
	enum class EventType : uint16_t
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	enum class EventCategory : uint16_t
	{
		None = 0,
		Application		= BIT(0),
		Input			= BIT(1),
		Keyboard		= BIT(2),
		Mouse			= BIT(3),
		MouseButton		= BIT(4)
	};

	struct Event
	{
		EventType type;
		EventCategory categories;

		uint16_t x;
		uint16_t y;

		//constructor type functions
		static Event windowResize(uint16_t width, uint16_t height);
		static Event windowClose();
	};
}