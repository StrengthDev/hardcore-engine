#pragma once

#include "Core.hpp"
#include "Event.hpp"

#include <functional>

namespace Spiral
{
	class SPIRAL_API Window
	{
	public:
		using fnEventCallback = std::function<void(Event)>;

		virtual ~Window() = default;

		virtual void getDimensions(int* width, int* height) const = 0;

		virtual void setEventCallback(const fnEventCallback& callback) = 0;

		virtual void setTitle(const char* title) = 0;

		virtual void setIcon(const char** files, unsigned int num) = 0;

		virtual void tick() = 0;

		static Window* init();
	};
}