#pragma once

#include <variant>

#include <volk.h>

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

namespace hc::render::device {
	struct WindowDestructionMark {
		VkInstance instance;
		GLFWwindow *window;
	};

	struct SwapchainDestructionMark {
		GLFWwindow *window;
	};

	struct ResourceDestructionMark {
		VkBufferUsageFlags usage;
		u64 pool;
		VkDeviceSize offset;
		bool dynamic;
	};

	/**
	* @brief A variant which may contain any of the deletion marks.
	*/
	typedef std::variant<WindowDestructionMark, SwapchainDestructionMark, ResourceDestructionMark> DestructionMark;

	/**
	* @brief A helper type used for visiting a variant.
	*
	* @tparam Ts The types of the function overloads, these are typically inferred from the visit call. A function
	* which deduces it's argument using `auto` may be used for a "default" handler.
	*/
	template<class... Ts>
	struct DestructionMarkHandler : Ts... {
		using Ts::operator()...;
	};
}
