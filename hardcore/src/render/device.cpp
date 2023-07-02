#include <pch.hpp>

#include <render/device.hpp>
#include <render/shader_library.hpp>

#include <debug/log_internal.hpp>

namespace ENGINE_NAMESPACE
{
	inline void calc_queue_indices(VkPhysicalDevice& physical_handle, VkSurfaceKHR& surface,
		std::uint32_t* out_graphics_idx, std::uint32_t* out_present_idx, std::uint32_t* out_compute_idx, std::uint32_t* out_transfer_idx,
		std::uint32_t invalid_idx)
	{
		std::uint32_t graphics_idx = invalid_idx, present_idx = invalid_idx, compute_idx = invalid_idx, transfer_idx = invalid_idx,
			graphics_score = 0, present_score = 0, compute_score = 0, transfer_score = 0;

		std::uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_handle, &queue_family_count, nullptr);
		VkQueueFamilyProperties* queue_families = t_calloc<VkQueueFamilyProperties>(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(physical_handle, &queue_family_count, queue_families);

		for (std::uint32_t i = 0; i < queue_family_count; i++)
		{
			if (queue_families[i].queueCount)
			{
				if (graphics_score < 5 && (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
				{
					graphics_idx = i;
					graphics_score = 5;
				}

				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(physical_handle, i, surface, &presentSupport);
				if (present_score < 10 && presentSupport)
				{
					present_idx = i;
					present_score = 5;
					if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					{
						//Optimal if both graphics and present queues are in the same family
						graphics_idx = i;
						graphics_score = 10;
						present_score = 10;
					}
				}

				if (compute_score < 5 && (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
				{
					compute_idx = i;
					compute_score = 5;
				}

				if (compute_score < 10 && (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && 
					!(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
				{
					//Async compute queue is ideal
					compute_idx = i;
					compute_score = 10;
				}

				if (transfer_score < 3 && (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT))
				{
					transfer_idx = i;
					transfer_score = 3;
				}

				if (transfer_score < 6 && (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && 
					!(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
				{
					transfer_idx = i;
					transfer_score = 6;
				}

				if (transfer_score < 10 && (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && 
					!(queue_families[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)))
				{
					//Async transfer is ideal
					transfer_idx = i;
					transfer_score = 10;
				}

				LOG_INTERNAL_INFO("[RENDERER] Queue family " << i << " properties: " << queue_families[i].queueCount << ' '
					//<< '(' << std::bitset<sizeof(VkQueueFlags) * 8>(queue_families[i].queueFlags) << ") => "
					<< (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ? "GRAPHICS " : "")
					<< (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT ? "COMPUTE " : "")
					<< (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT ? "TRANSFER " : "")
					<< (queue_families[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT ? "SPARSE_BINDING " : "")
					<< (presentSupport ? "PRESENT " : ""));
			}
		}
		if (out_graphics_idx) *out_graphics_idx = graphics_idx;
		if (out_present_idx) *out_present_idx = present_idx;
		if (out_compute_idx) *out_compute_idx = compute_idx;
		if (out_transfer_idx) *out_transfer_idx = transfer_idx;
		std::free(queue_families);

		LOGF_INTERNAL_INFO("[RENDERER] Selected queue indices: graphics = {0} | present = {1} | compute = {2} | transfer = {3}",
			graphics_idx, present_idx, compute_idx, transfer_idx);
	}

	inline void create_logical_device(VkPhysicalDevice& physical_handle, VkDevice& handle,
		VkQueue* graphics_queue, VkQueue* present_queue, VkQueue* compute_queue, VkQueue* transfer_queue,
		std::uint32_t graphics_idx, std::uint32_t present_idx, std::uint32_t compute_idx, std::uint32_t transfer_idx, 
		std::uint32_t invalid_idx)
	{
		std::set<std::uint32_t> unique_queue_families = { graphics_idx, present_idx, compute_idx, transfer_idx };
		VkDeviceQueueCreateInfo* queue_create_infos = t_calloc<VkDeviceQueueCreateInfo>(unique_queue_families.size());
		std::uint32_t count = 0;
		float queue_priority = 1.0f;
		for (std::uint32_t index : unique_queue_families)
		{
			if (index != invalid_idx)
			{
				VkDeviceQueueCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				info.queueFamilyIndex = index;
				info.queueCount = 1;
				info.pQueuePriorities = &queue_priority;
				queue_create_infos[count] = info;
				count++;
			}
		}

		VkPhysicalDeviceFeatures device_features = {}; //TODO: idk

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queue_create_infos;
		createInfo.queueCreateInfoCount = count;
		createInfo.pEnabledFeatures = &device_features;
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

		VK_CRASH_CHECK(vkCreateDevice(physical_handle, &createInfo, nullptr, &handle), "Failed to create logical device handle");

		std::free(queue_create_infos);

		if(graphics_idx != invalid_idx)
			vkGetDeviceQueue(handle, graphics_idx, 0, graphics_queue);
		if (present_idx != invalid_idx)
			vkGetDeviceQueue(handle, present_idx, 0, present_queue);
		if (compute_idx != invalid_idx)
			vkGetDeviceQueue(handle, compute_idx, 0, compute_queue);
		if (transfer_idx != invalid_idx)
			vkGetDeviceQueue(handle, transfer_idx, 0, transfer_queue);
	}

	inline void create_command_buffers(VkDevice& handle,  std::uint32_t command_parallelism, 
		std::uint32_t graphics_idx, VkCommandPool** graphics_command_pools, VkCommandBuffer** graphics_command_buffers)
	{
		*graphics_command_pools = t_malloc<VkCommandPool>(command_parallelism);
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = graphics_idx;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		for (std::uint32_t i = 0; i < command_parallelism; i++)
			VK_CRASH_CHECK(vkCreateCommandPool(handle, &poolInfo, nullptr, &(*graphics_command_pools)[i]), 
				"Failed to create graphics command pool");
		
		const std::uint32_t buffers_per_frame = command_parallelism + 1;
		*graphics_command_buffers = t_malloc<VkCommandBuffer>(static_cast<std::size_t>(max_frames_in_flight) * buffers_per_frame);
		for (std::uint32_t f = 0; f < max_frames_in_flight; f++)
		{
			//Primary buffer allocation
			VkCommandBufferAllocateInfo mainAllocInfo = {};
			mainAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			mainAllocInfo.commandPool = (*graphics_command_pools)[0];
			mainAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			mainAllocInfo.commandBufferCount = 1;

			VK_CRASH_CHECK(vkAllocateCommandBuffers(handle, &mainAllocInfo, &(*graphics_command_buffers)[buffers_per_frame * f]), 
				"Failed to allocate main graphics command buffer");

			//Secondary buffer allocation
			for (std::uint32_t i = 0; i < command_parallelism; i++)
			{
				VkCommandBufferAllocateInfo subAllocInfo = {};
				subAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				subAllocInfo.commandPool = (*graphics_command_pools)[i];
				subAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
				subAllocInfo.commandBufferCount = 1;
				
				VK_CRASH_CHECK(vkAllocateCommandBuffers(handle, &subAllocInfo, &(*graphics_command_buffers)[buffers_per_frame * f + i + 1]),
					"Failed to allocate secondary graphics command buffer");
			}
		}
	}

	device::device(VkPhysicalDevice&& physical, VkSurfaceKHR& surface) : surface(&surface), physical_handle(std::move(physical)),
		command_parallelism(1), pipeline_stacks(1) //TODO: + immediate_thread_num()
	{
		vkGetPhysicalDeviceProperties(physical_handle, &properties);
		vkGetPhysicalDeviceFeatures(physical_handle, &features);

		LOGF_INTERNAL_INFO("[RENDERER] Logical device being created for {0} ..", properties.deviceName);

		calc_queue_indices(physical_handle, surface, &graphics_idx, &present_idx, &compute_idx, &transfer_idx, invalid_queue_idx);
		
		//TODO: verify extension compatibility

		create_logical_device(physical_handle, handle, &graphics_queue, &present_queue, &compute_queue, &transfer_queue,
			graphics_idx, present_idx, compute_idx, transfer_idx, invalid_queue_idx);

		create_command_buffers(handle, command_parallelism, graphics_idx, &graphics_command_pools, &graphics_command_buffers);

		memory.init(physical_handle, handle, transfer_idx, &properties.limits, &current_frame);
		memory.map_ranges(handle, current_frame);
		pipeline_stacks.resize(1);
	}

	device::~device()
	{
		while (!old_framebuffers.empty())
		{
			old_framebuffer_set& old = old_framebuffers.front();
			if (old.deletion_frame == current_frame)
			{
				for (std::size_t i = 0; i < old.n_framebuffers; i++)
					vkDestroyFramebuffer(handle, old.framebuffers[i], nullptr);
				std::free(old.framebuffers);
				old_framebuffers.pop();
			}
		}

		if (has_swapchain)
		{
			vkDeviceWaitIdle(handle);
			//Pipelines must be destroyed before anything else
			graphics_pipelines.clear();
			for (std::size_t i = 0; i < main_swapchain.size(); i++)
				vkDestroyFramebuffer(handle, framebuffers[i], nullptr);
			std::free(framebuffers);
			vkDestroyRenderPass(handle, render_pass, nullptr);
			main_swapchain.terminate(handle);
			for (std::size_t i = 0; i < max_frames_in_flight; i++)
			{
				vkDestroySemaphore(handle, render_finished_semaphores[i], nullptr);
				vkDestroySemaphore(handle, image_available_semaphores[i], nullptr);
				vkDestroyFence(handle, frame_fences[i], nullptr);
			}
			has_swapchain = false;
		}
		
		if (handle != VK_NULL_HANDLE)
		{
			vkDeviceWaitIdle(handle);
			memory.terminate(handle, current_frame);
			for (std::size_t i = 0; i < command_parallelism; i++)
			{
				//Destroying a command pool frees all of its buffers as well
				vkDestroyCommandPool(handle, graphics_command_pools[i], nullptr);
			}
			std::free(graphics_command_pools);
			std::free(graphics_command_buffers);
			vkDestroyDevice(handle, nullptr);
			handle = VK_NULL_HANDLE;
		}
	}

	std::size_t device::score()
	{
		return 0; //TODO
	}

	void device::create_swapchain()
	{
		main_swapchain.init(physical_handle, handle, *surface, graphics_idx, present_idx);
		has_swapchain = true;

		VkSemaphoreCreateInfo semaphore_info = {};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fence_info = {};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		for (std::size_t i = 0; i < max_frames_in_flight; i++)
		{
			VK_CRASH_CHECK(vkCreateSemaphore(handle, &semaphore_info, nullptr, &image_available_semaphores[i]), 
				"Failed to create image semaphore");
			VK_CRASH_CHECK(vkCreateSemaphore(handle, &semaphore_info, nullptr, &render_finished_semaphores[i]), 
				"Failed to create render semaphore");
			VK_CRASH_CHECK(vkCreateFence(handle, &fence_info, nullptr, &frame_fences[i]), "Failed to create frame fence");
		}

		VkAttachmentDescription attachment = {};
		attachment.format = main_swapchain.image_format();
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;	//multisampling
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;	//define pixel layout of VkImages in memory

		VkAttachmentReference attachment_ref = {};
		attachment_ref.attachment = 0;
		attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &attachment_ref;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = 1;
		render_pass_info.pAttachments = &attachment;
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;
		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies = &dependency;

		VK_CRASH_CHECK(vkCreateRenderPass(handle, &render_pass_info, nullptr, &render_pass), 
			"Failed to create render pass");

		framebuffers = t_malloc<VkFramebuffer>(main_swapchain.size());
		for (std::size_t i = 0; i < main_swapchain.size(); i++)
		{
			VkFramebufferCreateInfo framebuffer_info = {};
			framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_info.renderPass = render_pass;
			framebuffer_info.attachmentCount = 1;
			framebuffer_info.pAttachments = &main_swapchain.views()[i];
			framebuffer_info.width = main_swapchain.extent().width;
			framebuffer_info.height = main_swapchain.extent().height;
			framebuffer_info.layers = 1;

			VK_CRASH_CHECK(vkCreateFramebuffer(handle, &framebuffer_info, nullptr, &framebuffers[i]), 
				"Failed to create framebuffer");
		}
	}

	void device::recreate_swapchain()
	{
		old_framebuffer_set old = {};
		old.framebuffers = framebuffers;
		old.n_framebuffers = main_swapchain.size();
		old.deletion_frame = current_frame;

		old_framebuffers.push(std::move(old));

		main_swapchain.recreate(physical_handle, handle, *surface, current_frame);

		framebuffers = t_malloc<VkFramebuffer>(main_swapchain.size());
		for (std::size_t i = 0; i < main_swapchain.size(); i++)
		{
			VkFramebufferCreateInfo framebuffer_info = {};
			framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_info.renderPass = render_pass;
			framebuffer_info.attachmentCount = 1;
			framebuffer_info.pAttachments = &main_swapchain.views()[i];
			framebuffer_info.width = main_swapchain.extent().width;
			framebuffer_info.height = main_swapchain.extent().height;
			framebuffer_info.layers = 1;

			VK_CRASH_CHECK(vkCreateFramebuffer(handle, &framebuffer_info, nullptr, &framebuffers[i]), 
				"Failed to create framebuffer");
		}
	}

	void device::record_secondary_graphics(VkCommandBuffer& buffer, std::uint32_t image_index, 
		std::vector<graphics_pipeline*>& graphics_pipelines)
	{
		VK_CRASH_CHECK(vkResetCommandBuffer(buffer, 0), "Failed to reset secondary graphics command buffer");

		VkCommandBufferInheritanceInfo inheritance_info = {};
		inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritance_info.pNext = nullptr;
		inheritance_info.renderPass = render_pass;
		inheritance_info.subpass = 0;
		inheritance_info.framebuffer = framebuffers[image_index];
		inheritance_info.occlusionQueryEnable = VK_FALSE;
		inheritance_info.queryFlags = 0;
		inheritance_info.pipelineStatistics = 0;

		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		begin_info.pInheritanceInfo = &inheritance_info;

		VK_CRASH_CHECK(vkBeginCommandBuffer(buffer, &begin_info), "Failed to begin secondary graphics command buffer");

		vkCmdSetViewport(buffer, 0, 1, &main_swapchain.viewport());
		vkCmdSetScissor(buffer, 0, 1, &main_swapchain.scissor());

		for (graphics_pipeline* pipeline : graphics_pipelines)
		{
			pipeline->record_commands(buffer, current_frame);
		}

		VK_CRASH_CHECK(vkEndCommandBuffer(buffer), "Failed to end secondary graphics command buffer");
		graphics_pipelines.clear();
	}

	bool device::draw()
	{
		const std::uint8_t next_frame = (current_frame + 1) % max_frames_in_flight;
		const std::uint8_t previous_frame = (current_frame - 1 + max_frames_in_flight) % max_frames_in_flight;

		vkWaitForFences(handle, 1, &frame_fences[current_frame], VK_TRUE, UINT64_MAX);
		for (auto& pipeline : graphics_pipelines) pipeline.update_descriptor_sets(previous_frame, current_frame, next_frame); //TODO consider parallelizing this
		const bool memory_flushed = memory.flush_in(handle, transfer_queue, current_frame);
		memory.unmap_ranges(handle, current_frame);
		main_swapchain.check_destroy_old(handle, current_frame);
		if (!old_framebuffers.empty())
		{
			old_framebuffer_set& old = old_framebuffers.front();
			if (old.deletion_frame == current_frame)
			{
				for (std::size_t i = 0; i < old.n_framebuffers; i++)
					vkDestroyFramebuffer(handle, old.framebuffers[i], nullptr);
				std::free(old.framebuffers);
				old_framebuffers.pop();
			}
		}

		std::uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(handle, main_swapchain.vk_handle(), UINT64_MAX, image_available_semaphores[current_frame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreate_swapchain();
			return true; //Skip this frame
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			DEBUG_BREAK;
			return false;
		}
		
		VkCommandBuffer& primary_buffer = graphics_command_buffers[current_frame * (command_parallelism + 1)];

		VK_CRASH_CHECK(vkResetCommandBuffer(primary_buffer, 0), "Failed to reset primary graphics command buffer");

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		VK_CRASH_CHECK(vkBeginCommandBuffer(primary_buffer, &beginInfo), "Failed to begin primary graphics command buffer");

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = render_pass;
		renderPassInfo.framebuffer = framebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = main_swapchain.extent();

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		std::uint32_t index = 0;
		for (graphics_pipeline& pipeline : graphics_pipelines)
		{
			pipeline_stacks[index].push_back(&pipeline);
			index = (index + 1) % command_parallelism;
		}

		vkCmdBeginRenderPass(primary_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);// VK_SUBPASS_CONTENTS_INLINE);
		for (std::uint32_t i = 0; i < command_parallelism; i++)
		{
			record_secondary_graphics(graphics_command_buffers[(command_parallelism + 1) * current_frame + i + 1], 
				imageIndex, pipeline_stacks[i]);
		}

		//TODO: wait for thread i to finish recording
		vkCmdExecuteCommands(primary_buffer, command_parallelism, &graphics_command_buffers[(command_parallelism + 1) * current_frame + 1]);

		vkCmdEndRenderPass(primary_buffer);

		VK_CRASH_CHECK(vkEndCommandBuffer(primary_buffer), "Failed to end primary graphics command buffer");

		std::vector<VkSemaphore> pre_render_semaphores;
		pre_render_semaphores.reserve(2);
		pre_render_semaphores.push_back(image_available_semaphores[current_frame]);
		if (memory_flushed) pre_render_semaphores.push_back(memory.get_device_in_semaphore(current_frame));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
		submitInfo.waitSemaphoreCount = static_cast<std::uint32_t>(pre_render_semaphores.size());
		submitInfo.pWaitSemaphores = pre_render_semaphores.data();
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &primary_buffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &render_finished_semaphores[current_frame];

		vkResetFences(handle, 1, &frame_fences[current_frame]);
		VK_CRASH_CHECK(vkQueueSubmit(graphics_queue, 1, &submitInfo, frame_fences[current_frame]), "Failed to submit render commands");

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &render_finished_semaphores[current_frame];
		VkSwapchainKHR swapchains[] = { main_swapchain.vk_handle() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional

		result = vkQueuePresentKHR(present_queue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			//DEBUG_BREAK;
			recreate_swapchain();
		}
		else if (result != VK_SUCCESS)
		{
			DEBUG_BREAK;
			return false;
		}
		current_frame = next_frame;
		memory.synchronize(handle, current_frame); //wait for the next frame's data transfers to finish, so nothing can be overwritten by the client
		memory.map_ranges(handle, current_frame);
		return true;
	}

	std::uint32_t device::add_graphics_pipeline(const shader& vertex, const shader& fragment)
	{

		const std::vector<const shader*> shaders = { &vertex, &fragment };
		graphics_pipelines.push_back(graphics_pipeline(*this, shaders));
		return static_cast<std::uint32_t>(graphics_pipelines.size() - 1);
	}

	void device::add_instanced_graphics_pipeline()
	{

	}

	void device::add_indirect_graphics_pipeline()
	{

	}
}
