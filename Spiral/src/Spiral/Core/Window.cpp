#include "pch.hpp"

#include "WindowObject.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Spiral
{
	void errorCallback(int error, const char* description)
	{
		SPRL_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

	WindowObject::WindowObject()
	{
		int init = glfwInit();
		SPRL_CORE_DEBUG("Attempted to initialize GLFW, code ({0})", init); //code 1 = good
		glfwSetErrorCallback(errorCallback);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(1080, 720, "Spiral Engine", nullptr, nullptr);
		glfwSetWindowUserPointer(window, &data);

		glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height)
		{
			EventCallbackContainer& container = *(EventCallbackContainer*)glfwGetWindowUserPointer(window);
			//TODO: complete the callback
			container.eventCallback(Event::windowResize(width, height));
		});

		glfwSetWindowCloseCallback(window, [](GLFWwindow* window)
		{
			EventCallbackContainer& container = *(EventCallbackContainer*)glfwGetWindowUserPointer(window);
			container.eventCallback(Event::windowClose());
		});

		glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			//TODO: complete the callback
		});

		glfwSetCharCallback(window, [](GLFWwindow* window, unsigned int keycode)
		{
			//TODO: complete the callback
		});

		glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods)
		{
			//TODO: complete the callback
		});

		glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset)
		{
			//TODO: complete the callback
		});

		glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos)
		{
			//TODO: complete the callback
		});
	}

	WindowObject::~WindowObject()
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void WindowObject::getDimensions(int* width, int* height) const
	{
		glfwGetFramebufferSize(window, width, height);
	}

	void WindowObject::setEventCallback(const fnEventCallback& callback)
	{
		data.eventCallback = callback;
	}

	void WindowObject::setTitle(const char* title)
	{
		glfwSetWindowTitle(window, title);
	}

	void WindowObject::setIcon(const char** files, unsigned int num)
	{
		std::vector<GLFWimage> images(num);
		unsigned int i;
		for (i = 0; i < num; i++)
		{
			images[i].pixels = stbi_load(files[i], &images[i].width, &images[i].height, 0, 4);
		}
		glfwSetWindowIcon(window, 2, images.data());
		for (i = 0; i < num; i++)
		{
			stbi_image_free(images[i].pixels);
		}
	}

	void WindowObject::tick()
	{
		glfwPollEvents();
	}

	Window* Window::init()
	{
		return new WindowObject();
	}
}