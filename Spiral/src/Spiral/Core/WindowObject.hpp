#pragma once

#include "Window.hpp"

#include <GLFW/glfw3.h>

namespace Spiral
{
	class WindowObject : public Window
	{
	public:
		WindowObject();

		virtual ~WindowObject();

		inline void getDimensions(int *width, int *height) const override;

		inline void setEventCallback(const fnEventCallback& callback) override;
		inline void setSizeCallback(const fnSizeCallback &callback) override;

		inline void setTitle(const char *title) override;

		inline void setIcon(const char **files, unsigned int num) override;

		void tick() override;

	private:
		GLFWwindow* window;

		struct EventCallbackContainer
		{
			fnEventCallback eventCallback;
			fnSizeCallback sizeCallback;
		};

		EventCallbackContainer callbackContainer;
	};
}