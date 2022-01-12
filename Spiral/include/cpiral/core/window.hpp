#pragma once

#include "core.hpp"
#include "event.hpp"

#include <functional>

namespace Spiral
{
	class SPIRAL_API Window
	{
	public:
		using fnEventCallback = std::function<void(Event)>;
		using fnSizeCallback = std::function<void(int, int)>;

		virtual ~Window() = default;

		const virtual void getDimensions(int *width, int *height) const = 0;

		virtual void setEventCallback(const fnEventCallback &callback) = 0;
		virtual void setSizeCallback(const fnSizeCallback &callback) = 0;

		virtual void setTitle(const char *title) = 0;
		virtual void setIcon(const char **files, unsigned int num) = 0;

		virtual void tick() = 0;

		virtual void* getInstance() const = 0;


		static Window* init();
		//TODO: static functions
	};
}