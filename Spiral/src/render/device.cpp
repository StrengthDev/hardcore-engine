#include <pch.hpp>

#include <spiral/render/device.hpp>
#include <spiral/render/shader_library.hpp>

namespace Spiral
{
	device::device(VkPhysicalDevice physical, VkSurfaceKHR surface) : surface(surface), physical_handle(physical), n_graphics_command_pools(1) //+ immediate_thread_num()
	{
		uint32_t i;
		vkGetPhysicalDeviceProperties(physical_handle, &properties);
		vkGetPhysicalDeviceFeatures(physical_handle, &features);

		LOGF_INTERNAL_INFO("[RENDERER] Physical device found: {0}", properties.deviceName);

		queue_idx_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueFamilyCount, nullptr);
		VkQueueFamilyProperties* queueFamilies = t_malloc<VkQueueFamilyProperties>(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueFamilyCount, queueFamilies);
		for (i = 0; i < queueFamilyCount; i++) //TODO: Performance is better if both graphics and present queues are in the same family, also should probably optimize queue selection
		{
			if (queueFamilies[i].queueCount && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) //TODO: graphis support implies compute and transfer support
			{
				graphicsIndex = i;
			}

			if (queueFamilies[i].queueCount && queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				computeIndex = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physical, i, surface, &presentSupport);
			if (queueFamilies[i].queueCount && presentSupport) //TODO: present support implies graphic support
			{
				presentIndex = i;
			}

			if (graphicsIndex != invalid_queue_idx && presentIndex != invalid_queue_idx && computeIndex != invalid_queue_idx)
			{
				break;
			}
		}
		//TODO: verify extension compatibility

		i = 0;
		std::set<queue_idx_t> uniqueQueueFamilies = { graphicsIndex, presentIndex }; //TODO: no need to use a set, make an array with the max possible amount of different queue types and check if the one youre adding is already in the array, then malloc an array with the result
		VkDeviceQueueCreateInfo* queueCreateInfos = t_malloc<VkDeviceQueueCreateInfo>(uniqueQueueFamilies.size());
		float queuePriority = 1.0f;
		for (queue_idx_t index : uniqueQueueFamilies)
		{
			if (index != invalid_queue_idx)
			{
				VkDeviceQueueCreateInfo info = {}; //TODO: make Queue struct, look at queue selection in DEV bookmarks
				info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				info.queueFamilyIndex = index;
				info.queueCount = 1;
				info.pQueuePriorities = &queuePriority;
				queueCreateInfos[i] = info;
				i++;
			}
		}
		
		VkPhysicalDeviceFeatures deviceFeatures = {}; //TODO: idk
		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos;
		createInfo.queueCreateInfoCount = i;
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
		createInfo.ppEnabledExtensionNames = device_extensions.data();
		if (enable_validation_layers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
			createInfo.ppEnabledLayerNames = validation_layers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}
		if (vkCreateDevice(physical, &createInfo, nullptr, &handle) == VK_SUCCESS) //This looks wrong and stupid
		{
			has_handle = true;
			if (graphicsIndex != invalid_queue_idx)
			{
				vkGetDeviceQueue(handle, graphicsIndex, 0, &graphicsQueue);
			}
			if (presentIndex != invalid_queue_idx)
			{
				vkGetDeviceQueue(handle, presentIndex, 0, &presentQueue);
			}
			if (computeIndex != invalid_queue_idx)
			{
				vkGetDeviceQueue(handle, computeIndex, 0, &computeQueue);
			}
		}
		//SPRL_CORE_TRACE("Graphics family index: {0}", graphicsIndex);
		//SPRL_CORE_TRACE("Present family index: {0}", presentIndex);
		//SPRL_CORE_TRACE("Compute family index: {0}", computeIndex);
		std::free(queueFamilies);
		std::free(queueCreateInfos);
		transfer_idx = graphicsIndex;
		transfer_queue = graphicsQueue; //TODO

		graphics_command_pools = t_malloc<VkCommandPool>(n_graphics_command_pools);
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = graphicsIndex;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional
		for (i = 0; i < n_graphics_command_pools; i++)
		{
			if (vkCreateCommandPool(handle, &poolInfo, nullptr, &graphics_command_pools[i]) != VK_SUCCESS)
			{
				//do stuff
				DEBUG_BREAK;
			}
		}
		//TODO: could try reaaranging the buffer positions to be contiguous for a frame instead of type
		graphics_command_buffers = t_malloc<VkCommandBuffer>(static_cast<size_t>(max_frames_in_flight) * n_graphics_command_pools);
		VkCommandBufferAllocateInfo mainAllocInfo = {};
		mainAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		mainAllocInfo.commandPool = graphics_command_pools[0];
		mainAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		mainAllocInfo.commandBufferCount = max_frames_in_flight;
		if (vkAllocateCommandBuffers(handle, &mainAllocInfo, graphics_command_buffers) != VK_SUCCESS)
		{
			//TODO: cleanup properly
			DEBUG_BREAK;
		}
		for (i = 1; i < n_graphics_command_pools; i++)
		{
			VkCommandBufferAllocateInfo subAllocInfo = {};
			subAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			subAllocInfo.commandPool = graphics_command_pools[i];
			subAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
			subAllocInfo.commandBufferCount = max_frames_in_flight;
			if (vkAllocateCommandBuffers(handle, &subAllocInfo, &graphics_command_buffers[max_frames_in_flight]) != VK_SUCCESS)
			{
				//TODO: cleanup properly
				DEBUG_BREAK;
			}
		}

		//Memory::init(&physical_handle, &handle, &graphicsQueue, &graphics_command_pools[0]);
		memory.init(*this);
	}

	device::~device()
	{
		size_t i;
		if (has_swapchain)
		{
			vkDeviceWaitIdle(handle);
			for (i = 0; i < n_graphics_pipelines; i++)
			{
				graphics_pipelines[i].~graphics_pipeline();
			}
			for (i = 0; i < main_swapchain.n_images; i++)
			{
				vkDestroyFramebuffer(handle, framebuffers[i], nullptr);
			}
			std::free(framebuffers);
			vkDestroyRenderPass(handle, render_pass, nullptr);
			main_swapchain.terminate();
			for (i = 0; i < max_frames_in_flight; i++)
			{
				vkDestroySemaphore(handle, render_finished_semaphores[i], nullptr);
				vkDestroySemaphore(handle, image_available_semaphores[i], nullptr);
				vkDestroyFence(handle, frame_fences[i], nullptr);
			}
			has_swapchain = false;
		}
		//Memory::terminate();
		if (has_handle)
		{
			vkDeviceWaitIdle(handle);
			memory.terminate();
			for (i = 0; i < n_graphics_command_pools; i++)
			{
				vkDestroyCommandPool(handle, graphics_command_pools[i], nullptr); //Destroying a command pool frees all of its buffers as well
			}
			std::free(graphics_command_pools);
			std::free(graphics_command_buffers);
			vkDestroyDevice(handle, nullptr);
			has_handle = false;
		}
	}

	bool device::create_swapchain()
	{
		main_swapchain.init(*this);
		has_swapchain = true;

		size_t i;
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		for (i = 0; i < max_frames_in_flight; i++)
		{
			if (vkCreateSemaphore(handle, &semaphoreInfo, nullptr, &image_available_semaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(handle, &semaphoreInfo, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS ||
				vkCreateFence(handle, &fenceInfo, nullptr, &frame_fences[i]) != VK_SUCCESS)
			{
				return false; //TODO: destroy objects that might have already been created
			}
		}

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = main_swapchain.image_format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;	//multisampling
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;	//define pixel layout of VkImages in memory

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(handle, &renderPassInfo, nullptr, &render_pass) != VK_SUCCESS)
		{
			DEBUG_BREAK;
			return false;
		}

		framebuffers = t_malloc<VkFramebuffer>(main_swapchain.n_images);
		for (i = 0; i < main_swapchain.n_images; i++)
		{
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = render_pass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &main_swapchain.image_views[i];
			framebufferInfo.width = main_swapchain.extent.width;
			framebufferInfo.height = main_swapchain.extent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(handle, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
			{
				DEBUG_BREAK;
				return false;
			}
		}

		return true;
	}

	bool device::recreate_swapchain()
	{
		return true;
	}

	void device::record_secondary_graphics(index_t buffer_idx, uint32_t image_index)
	{
		if (vkResetCommandBuffer(graphics_command_buffers[buffer_idx], 0) != VK_SUCCESS)
		{
			//do stuff
			DEBUG_BREAK;
		}

		VkCommandBufferInheritanceInfo inheritanceInfo = {};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.pNext = nullptr;
		inheritanceInfo.renderPass = render_pass;
		inheritanceInfo.subpass = 0;
		inheritanceInfo.framebuffer = framebuffers[image_index];
		inheritanceInfo.occlusionQueryEnable = VK_FALSE;
		inheritanceInfo.queryFlags = 0;
		inheritanceInfo.pipelineStatistics = 0;

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = &inheritanceInfo; // Optional

		if (vkBeginCommandBuffer(graphics_command_buffers[current_frame], &beginInfo) != VK_SUCCESS)
		{
			//do stuff
			DEBUG_BREAK;
		}

		for (index_t i = (buffer_idx / max_frames_in_flight) - 1; i < n_graphics_pipelines; i += (n_graphics_command_pools - 1))
		{
			graphics_pipelines[i].record_commands(graphics_command_buffers[buffer_idx]);
		}

		if (vkEndCommandBuffer(graphics_command_buffers[buffer_idx]) != VK_SUCCESS)
		{
			//do stuff
			DEBUG_BREAK;
		}
	}

	bool device::draw()
	{
		memory.flush_in(current_frame);
		vkWaitForFences(handle, 1, &frame_fences[current_frame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		index_t i;
		VkResult result = vkAcquireNextImageKHR(handle, main_swapchain.handle, UINT64_MAX, image_available_semaphores[current_frame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			//TODO: recreate target
			return true; //Skip this frame
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			DEBUG_BREAK;
			return false;
		}

		if (vkResetCommandBuffer(graphics_command_buffers[current_frame], 0) != VK_SUCCESS)
		{
			//do stuff
			DEBUG_BREAK;
		}

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(graphics_command_buffers[current_frame], &beginInfo) != VK_SUCCESS)
		{
			//do stuff
			DEBUG_BREAK;
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = render_pass;
		renderPassInfo.framebuffer = framebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = main_swapchain.extent;

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(graphics_command_buffers[current_frame], &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);// VK_SUBPASS_CONTENTS_INLINE);
		for (i = 0; i < n_graphics_command_pools - 1; i++)
		{
			record_secondary_graphics((i + 1) * max_frames_in_flight + current_frame, imageIndex);
		}
		for (i = 0; i < n_graphics_command_pools - 1; i++)
		{
			//TODO: wait for thread i to finish recording
			vkCmdExecuteCommands(graphics_command_buffers[current_frame], 1, &graphics_command_buffers[(i + 1) * max_frames_in_flight + current_frame]);
		}
		vkCmdEndRenderPass(graphics_command_buffers[current_frame]);
		//TODO: update uniform buffer with imageIndex

		if (vkEndCommandBuffer(graphics_command_buffers[current_frame]) != VK_SUCCESS)
		{
			//do stuff
			DEBUG_BREAK;
		}

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &image_available_semaphores[current_frame];
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &graphics_command_buffers[current_frame];//&graphics_pipelines.commandBuffers[imageIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &render_finished_semaphores[current_frame];

		vkResetFences(handle, 1, &frame_fences[current_frame]);
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, frame_fences[current_frame]) != VK_SUCCESS) //TODO: porbably have to change this
		{
			DEBUG_BREAK;
			return false;
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &render_finished_semaphores[current_frame];
		VkSwapchainKHR swapchains[] = { main_swapchain.handle };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional

		result = vkQueuePresentKHR(presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			//TODO: recreate target, add framebuffer resized(?)
		}
		else if (result != VK_SUCCESS)
		{
			DEBUG_BREAK;
			return false;
		}
		current_frame = (current_frame + 1) % max_frames_in_flight;
		memory.synchronize(); //wait for the next frame's data transfers to finish, so nothing can be overwritten by the client
		return true;
	}
}
