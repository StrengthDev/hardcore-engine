#include <pch.hpp>

#include <core/event.hpp>

namespace ENGINE_NAMESPACE
{
	Event Event::windowResize(int64_t width, int64_t height)
	{
		Event res;
		res.type = EventType::WindowResize;
		res.categories = EventCategory::Application;
		res.x.i = width;
		res.y.i = height;
		return res;
	}

	Event Event::windowClose()
	{
		Event res;
		res.type = EventType::WindowClose;
		res.categories = EventCategory::Application;
		return res;
	}

	Event Event::keyPressed(int64_t key, int64_t scancode)
	{
		Event res;
		res.type = EventType::KeyPressed;
		res.categories = EventCategory::Input | EventCategory::Keyboard;
		res.x.i = key;
		res.y.i = scancode;
		return res;
	}

	Event Event::keyReleased(int64_t key, int64_t scancode)
	{
		Event res;
		res.type = EventType::KeyReleased;
		res.categories = EventCategory::Input | EventCategory::Keyboard;
		res.x.i = key;
		res.y.i = scancode;
		return res;
	}

	Event Event::keyTyped(uint64_t codepoint)
	{
		Event res;
		res.type = EventType::KeyTyped;
		res.categories = EventCategory::Input | EventCategory::Keyboard;
		res.x.u = codepoint;
		return res;
	}

	Event Event::mouseButtonPressed(int64_t button)
	{
		Event res;
		res.type = EventType::MouseButtonPressed;
		res.categories = EventCategory::Input | EventCategory::Mouse | EventCategory::MouseButton;
		res.x.i = button;
		return res;
	}

	Event Event::mouseButtonReleased(int64_t button)
	{
		Event res;
		res.type = EventType::MouseButtonReleased;
		res.categories = EventCategory::Input | EventCategory::Mouse | EventCategory::MouseButton;
		res.x.i = button;
		return res;
	}

	Event Event::mouseMoved(double xpos, double ypos)
	{
		Event res;
		res.type = EventType::MouseMoved;
		res.categories = EventCategory::Input | EventCategory::Mouse;
		res.x.f = xpos;
		res.y.f = ypos;
		return res;
	}

	Event Event::mouseScrolled(double xoffset, double yoffset)
	{
		Event res;
		res.type = EventType::MouseScrolled;
		res.categories = EventCategory::Input | EventCategory::Mouse;
		res.x.f = xoffset;
		res.y.f = yoffset;
		return res;
	}
}