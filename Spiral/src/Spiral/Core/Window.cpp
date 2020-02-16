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
		//SPRL_CORE_DEBUG("Attempted to initialize GLFW, code ({0})", init); //code 1 = good
		glfwSetErrorCallback(errorCallback);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(1080, 720, "Spiral Engine", nullptr, nullptr);
		glfwSetWindowUserPointer(window, &callbackContainer);

		glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height) //TODO: May need to add framebuffer callback, as it can differ on some machines
		{
			EventCallbackContainer& container = *(EventCallbackContainer*)glfwGetWindowUserPointer(window);
			container.sizeCallback(width, height);
		});

		glfwSetWindowCloseCallback(window, [](GLFWwindow* window)
		{
			EventCallbackContainer& container = *(EventCallbackContainer*)glfwGetWindowUserPointer(window);
			container.eventCallback(Event::windowClose());
		});

		glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			//TODO: might want to consider adding the mods to the Event object(shift, ctrl, alt and super)
			EventCallbackContainer& container = *(EventCallbackContainer*)glfwGetWindowUserPointer(window);
			switch (action)
			{
			case GLFW_PRESS:
				container.eventCallback(Event::keyPressed(key, scancode));
				return;
			case GLFW_RELEASE:
				container.eventCallback(Event::keyReleased(key, scancode));
				return;
			case GLFW_REPEAT:
				container.eventCallback(Event::keyPressed(key, scancode));
				return;
			}
		});

		glfwSetCharCallback(window, [](GLFWwindow* window, unsigned int codepoint)
		{
			EventCallbackContainer& container = *(EventCallbackContainer*)glfwGetWindowUserPointer(window);
			container.eventCallback(Event::keyTyped(codepoint));
		});

		glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods)
		{
			EventCallbackContainer& container = *(EventCallbackContainer*)glfwGetWindowUserPointer(window);
			switch (action)
			{
			case GLFW_PRESS:
				container.eventCallback(Event::mouseButtonPressed(button));
				return;
			case GLFW_RELEASE:
				container.eventCallback(Event::mouseButtonReleased(button));
				return;
			}
		});

		glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos)
		{
			//TODO: might want to optimize this callback in the same way as size callback, bloating eventbuffer without rendering
			EventCallbackContainer& container = *(EventCallbackContainer*)glfwGetWindowUserPointer(window);
			container.eventCallback(Event::mouseMoved(xpos, ypos));
		});

		glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset)
		{
			EventCallbackContainer& container = *(EventCallbackContainer*)glfwGetWindowUserPointer(window);
			container.eventCallback(Event::mouseScrolled(xoffset, yoffset));
		});
	}

	WindowObject::~WindowObject()
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	const void WindowObject::getDimensions(int *width, int *height) const
	{
		glfwGetFramebufferSize(window, width, height);
	}

	void WindowObject::setEventCallback(const fnEventCallback &callback)
	{
		callbackContainer.eventCallback = callback;
	}

	void WindowObject::setSizeCallback(const fnSizeCallback &callback)
	{
		callbackContainer.sizeCallback = callback;
	}

	void WindowObject::setTitle(const char *title)
	{
		glfwSetWindowTitle(window, title);
	}

	void WindowObject::setIcon(const char **files, unsigned int num)
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