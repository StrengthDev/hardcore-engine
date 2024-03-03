#pragma once

#include "core.hpp"

#include <cstdint>

namespace ENGINE_NAMESPACE
{
	enum class EventType : uint16_t
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	enum EventCategory : uint16_t
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
		uint16_t categories;

		union val64
		{
			int64_t i;
			uint64_t u;
			double f;
			char c;
			wchar_t w;
		};

		val64 x;
		val64 y;

		//constructor type functions
		static Event windowResize(int64_t width, int64_t height);
		static Event windowClose();
		static Event keyPressed(int64_t key, int64_t scancode);
		static Event keyReleased(int64_t key, int64_t scancode);
		static Event keyTyped(uint64_t codepoint);
		static Event mouseButtonPressed(int64_t button);
		static Event mouseButtonReleased(int64_t button);
		static Event mouseMoved(double xpos, double ypos);
		static Event mouseScrolled(double xoffset, double yoffset);
	};
}