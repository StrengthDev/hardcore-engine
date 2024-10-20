#pragma once

#include <optional>
#include <queue>

#include <volk.h>

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

#include <util/number.hpp>
#include <util/result.hpp>

namespace hc::render::device {
	enum class SwapchainResult : u8 {
		Success = 0,
		SkipFrame,
		FenceFailure,
		ImageAcquisitionFailure,
		CreationFailure,
		ImageViewFailure,
		UnsupportedSurface,
	};

	struct SurfaceInfo {
		VkSurfaceCapabilities2KHR capabilities = {};
		std::vector<VkSurfaceFormat2KHR> available_formats;
		std::vector<VkPresentModeKHR> available_present_modes;
	};

	struct SwapchainParams {
		u32 graphics_queue = std::numeric_limits<u32>::max();
		u32 present_queue = std::numeric_limits<u32>::max();
		VkExtent2D extent = VkExtent2D{.width = 0, .height = 0};
		VkPresentModeKHR preferred_present_mode = VK_PRESENT_MODE_FIFO_KHR;
		VkSurfaceFormatKHR preferred_format = VkSurfaceFormatKHR{
			.format = VK_FORMAT_B8G8R8A8_UNORM,
			.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		};
	};

	struct InnerSwapchain {
		VkSwapchainKHR handle = VK_NULL_HANDLE;
		std::vector<VkImageView> image_views;
	};

	class Swapchain {
	public:
		Swapchain(const Swapchain &) = delete;

		Swapchain &operator=(const Swapchain &) = delete;

		[[nodiscard]] static Result<Swapchain, SwapchainResult>
		create(const VolkDeviceTable &fn_table, VkDevice device, VkSurfaceKHR &&surface, SurfaceInfo &&surface_info,
				SwapchainParams &&params);

		~Swapchain();

		Swapchain(Swapchain &&other) noexcept;

		Swapchain &operator=(Swapchain &&other) noexcept;

		void destroy(VkInstance instance, const VolkDeviceTable &fn_table, VkDevice device);

		void destroy_old(const VolkDeviceTable &fn_table, VkDevice device);

		[[nodiscard]] Result<u32, SwapchainResult>
		acquire_image(const VolkDeviceTable &fn_table, VkDevice device, GLFWwindow *window, u8 frame_mod,
					u64 timeout = std::numeric_limits<u64>::max());

	private:
		// Hide default constructor, swapchains should be created using the factory function `Swapchain::create`.
		Swapchain() = default;

		/**
		* @brief Recreate the swapchain according to the new window specifications.
		*
		* @param fn_table The device function table.
		* @param device The device handle.
		* @param window The window from which the swapchain is created.
		* @return `SwapchainResult::Success` if the operation was successful, otherwise an error describing what went
		* wrong.
		*/
		SwapchainResult recreate(const VolkDeviceTable &fn_table, VkDevice device, GLFWwindow *window);

		InnerSwapchain inner; //!< The properties of a device's queue families.

		VkSurfaceKHR surface = VK_NULL_HANDLE; //!< The surface of the window from which the swapchain is created.
		/**
		* @brief The surface format to use.
		*/
		VkSurfaceFormatKHR surface_format = VkSurfaceFormatKHR{
			.format = VK_FORMAT_B8G8R8A8_UNORM,
			.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		};
		VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; //!< The presentation mode to use.

		VkExtent2D extent = VkExtent2D{.width = 0, .height = 0}; //!< The extent (dimensions) of the swapchain images.
		VkViewport viewport;
		VkRect2D scissor;

		/**
		* @brief The parameters used to create a new swapchain.
		*/
		struct CreationParams {
			u32 graphics_queue_family = std::numeric_limits<u32>::max();
			u32 present_queue_family = std::numeric_limits<u32>::max();
			u32 image_count = 0;
			VkSurfaceTransformFlagBitsKHR transform =
					VkSurfaceTransformFlagBitsKHR::VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR;
		} creation_params;

		/**
		* @brief Collection of fences used to sync access by the host CPU to the swapchain images.
		*
		* Size matches the maximum number of frames in flight.
		*/
		std::vector<VkFence> presentation_fences;
		/**
		* @brief Collection of semaphores used to sync access by the device to the swapchain images.
		*
		* Size matches the maximum number of frames in flight.
		*/
		std::vector<VkSemaphore> image_semaphores;

		// Old inner swapchains must not be destroyed while their resources are still in use.
		std::queue<InnerSwapchain> old_swapchains;
		//!< A queue containing old inner swapchains, in order of deprecation.
	};
}
