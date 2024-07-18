#include <pch.hpp>

#include <core/window.hpp>
#include <core/window_internal.hpp>

#include <debug/log_internal.hpp>

#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"

namespace ENGINE_NAMESPACE
{
	namespace window
	{
		GLFWwindow* window;
		event_callback_t event_callback;
		size_callback_t size_callback;

		void errorCallback(int error, const char* description)
		{
			LOGF_INTERNAL_ERROR("GLFW Error ({0}): {1}", error, description);
		}

		void init(event_callback_t&& ec, size_callback_t&& sc)
		{
			event_callback = ec;
			size_callback = sc;
			int init = glfwInit();
			//SPRL_CORE_DEBUG("Attempted to initialize GLFW, code ({0})", init); //code 1 = good
			glfwSetErrorCallback(errorCallback);
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

			window = glfwCreateWindow(1080, 720, "Spiral Engine", nullptr, nullptr);

			glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height) //TODO: May need to add framebuffer callback, as it can differ on some machines
				{
					size_callback(width, height);
				});

			glfwSetWindowCloseCallback(window, [](GLFWwindow* window)
				{
					event_callback(Event::windowClose());
				});

			glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
				{
					//TODO: might want to consider adding the mods to the Event object(shift, ctrl, alt and super)
					switch (action)
					{
					case GLFW_PRESS:
						event_callback(Event::keyPressed(key, scancode));
						return;
					case GLFW_RELEASE:
						event_callback(Event::keyReleased(key, scancode));
						return;
					case GLFW_REPEAT:
						event_callback(Event::keyPressed(key, scancode));
						return;
					}
				});

			glfwSetCharCallback(window, [](GLFWwindow* window, unsigned int codepoint)
				{
					event_callback(Event::keyTyped(codepoint));
				});

			glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods)
				{
					switch (action)
					{
					case GLFW_PRESS:
						event_callback(Event::mouseButtonPressed(button));
						return;
					case GLFW_RELEASE:
						event_callback(Event::mouseButtonReleased(button));
						return;
					}
				});

			glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos)
				{
					//TODO: might want to optimize this callback in the same way as size callback, bloating eventbuffer without rendering
					event_callback(Event::mouseMoved(xpos, ypos));
				});

			glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset)
				{
					event_callback(Event::mouseScrolled(xoffset, yoffset));
				});

			glfwSetDropCallback(window, [](GLFWwindow* window, int count, const char** paths)
				{
					for (int i = 0; i < count; i++)
					{
						LOG_INTERNAL_DEBUG("Path dropped: " << paths[i]) //TODO: actual event
					}
				});
		}

		void terminate()
		{
			glfwDestroyWindow(window);
			glfwTerminate();
		}

		void tick()
		{
			glfwPollEvents();
		}

		GLFWwindow* get_handle()
		{
			return window;
		}

		void get_dimensions(int* out_width, int* out_height)
		{
			glfwGetFramebufferSize(window, out_width, out_height);
		}

		void set_title(const char* title)
		{
			glfwSetWindowTitle(window, title);
		}

		void set_icon(const char** files, unsigned int num)
		{
			/*
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
			}*/
		}
	}
}
