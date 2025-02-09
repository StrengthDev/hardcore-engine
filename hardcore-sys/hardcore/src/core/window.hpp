#pragma once

#ifndef HC_HEADLESS

#include <core/glfw.hpp>

namespace hc::window {
	void destroy(GLFWwindow *window);

	bool is_resizing(GLFWwindow *window);
}

#endif // HC_HEADLESS
