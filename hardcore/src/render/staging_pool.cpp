#include <pch.hpp>

#include <render/staging_pool.hpp>

namespace ENGINE_NAMESPACE
{
	staging_pool::staging_pool(VkDevice device, device_heap_manager& heap_manager, VkDeviceSize size, 
		VkBufferUsageFlags usage, device_heap_manager::heap heap) :
		m_size(size), m_pending_size(0)
	{
		heap_manager.alloc_buffer(device, m_memory, m_buffer, size * max_frames_in_flight, usage, heap);
	}

	staging_pool::~staging_pool()
	{
		INTERNAL_ASSERT(!m_host_ptr, "Staging pool not unmapped");
		INTERNAL_ASSERT(m_memory == VK_NULL_HANDLE, "Staging pool not freed");
	}

	void staging_pool::free(VkDevice device, device_heap_manager& heap_manager)
	{
		if (m_memory != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, m_buffer, nullptr);
			m_buffer = VK_NULL_HANDLE;
			heap_manager.free(device, m_memory);
		}
	}

	void staging_pool::map(VkDevice device, u8 current_frame)
	{
		VK_CRASH_CHECK(vkMapMemory(device, m_memory, m_size * current_frame, m_size, 0, &m_host_ptr),
			"Failed to map memory");
	}

	void staging_pool::unmap(VkDevice device)
	{
		INTERNAL_ASSERT(m_host_ptr != nullptr, "Memmory not mapped");
		vkUnmapMemory(device, m_memory);
		m_host_ptr = nullptr;
	}

	void upload_pool::create_batch(u8 pool_type, std::size_t pool_idx, VkBuffer buffer)
	{
		INTERNAL_ASSERT(!has_batch(pool_type, pool_idx), "Batch already exists");

		m_pending_copies.insert({ { pool_type, pool_idx }, { buffer, std::vector<VkBufferCopy>() } });
	}

	void upload_pool::submit(u8 pool_type, std::size_t pool_idx, const void* data, VkDeviceSize size, VkDeviceSize offset)
	{
		INTERNAL_ASSERT(m_pending_size + size <= m_size, "Out of bounds memory access");
		INTERNAL_ASSERT(has_batch(pool_type, pool_idx), "Batch does not exist");

		m_pending_copies[{ pool_type, pool_idx }].regions.push_back({ 
			.srcOffset = m_pending_size, .dstOffset = offset, .size = size });

		std::memcpy(static_cast<std::byte*>(m_host_ptr) + m_pending_size, data, size);

		m_pending_size += size;
	}

	void upload_pool::record_and_clear(VkCommandBuffer& buffer)
	{
		if (!m_pending_size)
			return;

		for (const auto& [key, data] : m_pending_copies)
			vkCmdCopyBuffer(buffer, m_buffer, data.buffer, data.regions.size(), data.regions.data());
		
		m_pending_copies.clear();
		m_pending_size = 0;
	}

	inline VkImageMemoryBarrier layout_transition(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout,
		VkImageAspectFlags aspect, u32 queue_idx)
	{
		return {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ? VK_ACCESS_TRANSFER_WRITE_BIT : 0U,
			.dstAccessMask = new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ? VK_ACCESS_TRANSFER_WRITE_BIT : 0U,
			.oldLayout = old_layout,
			.newLayout = new_layout,
			.srcQueueFamilyIndex = queue_idx,
			.dstQueueFamilyIndex = queue_idx,
			.image = image,

			.subresourceRange = {
				.aspectMask = aspect,
				.baseMipLevel = 0,
				.levelCount = VK_REMAINING_MIP_LEVELS,
				.baseArrayLayer = 0,
				.layerCount = VK_REMAINING_ARRAY_LAYERS
			}
		};
	}

	void texture_upload_pool::buffer_image_copy(const void* data, VkDeviceSize size,
		VkImage image, VkImageLayout layout, VkExtent3D image_dims)
	{
		VkBufferImageCopy* regions = t_malloc<VkBufferImageCopy>(1);
		regions->bufferOffset = m_pending_size;
		regions->bufferRowLength = 0;
		regions->bufferImageHeight = 0;
		regions->imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		regions->imageSubresource.mipLevel = 0;
		regions->imageSubresource.baseArrayLayer = 0;
		regions->imageSubresource.layerCount = 1; //TODO do this properly
		regions->imageOffset = { 0, 0, 0 };
		regions->imageExtent = image_dims;

		m_pending_copies_transfer.push_back({
				.src = { .buffer = m_buffer },
				.src_layout_0 = VK_IMAGE_LAYOUT_UNDEFINED, // n/a
				.src_layout_1 = VK_IMAGE_LAYOUT_UNDEFINED, // n/a
				.dst_image = image,
				.dst_layout_0 = VK_IMAGE_LAYOUT_UNDEFINED,
				.dst_layout_1 = layout,
				.conversion = BUFFER_TO_IMAGE,
				.aspect = VK_IMAGE_ASPECT_COLOR_BIT,
				.regions = regions,
				.region_count = 1,
				.filter = VK_FILTER_MAX_ENUM // n/a
			});

		std::memcpy(static_cast<std::byte*>(m_host_ptr) + m_pending_size, data, size);
	}

	void texture_upload_pool::record_and_clear_transfer(VkCommandBuffer& buffer, u32 queue_idx)
	{
		if (!m_pending_copies_transfer.size())
			return;

		std::vector<VkImageMemoryBarrier> barriers_0, barriers_1;
		barriers_0.reserve(m_pending_copies_transfer.size());
		barriers_1.reserve(m_pending_copies_transfer.size());

		for (const transfer_data& data : m_pending_copies_transfer)
		{
			if (data.conversion == TEXEL_ONLY_CONVERSION)
			{
				barriers_0.push_back(layout_transition(data.src.image, 
					data.src_layout_0, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, data.aspect, queue_idx));
				barriers_1.push_back(layout_transition(data.src.image, 
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, data.src_layout_1, data.aspect, queue_idx));
			}

			barriers_0.push_back(layout_transition(data.dst_image, 
				data.dst_layout_0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, data.aspect, queue_idx));
			barriers_1.push_back(layout_transition(data.dst_image, 
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, data.dst_layout_1, data.aspect, queue_idx));

		}

		vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, barriers_0.size(), barriers_0.data());

		for (const transfer_data& data : m_pending_copies_transfer)
		{
			switch (data.conversion)
			{
			case BUFFER_TO_IMAGE:
				vkCmdCopyBufferToImage(buffer, data.src.buffer, data.dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
					data.region_count, static_cast<VkBufferImageCopy*>(data.regions));
				break;
			case TEXEL_ONLY_CONVERSION:
				vkCmdCopyImage(buffer, data.src.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
					data.dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					data.region_count, static_cast<VkImageCopy*>(data.regions));
				break;
			default:
				INTERNAL_ASSERT(false, "Invalid conversion");
				break;
			}

			std::free(data.regions);
		}

		vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, barriers_1.size(), barriers_1.data());

		m_pending_copies_transfer.clear();
		m_pending_size = 0;
	}

	void texture_upload_pool::record_and_clear_graphics(VkCommandBuffer& buffer, u32 queue_idx)
	{
		if (!m_pending_copies_graphics.size())
			return;

		for (const transfer_data& data : m_pending_copies_transfer)
		{
			switch (data.conversion)
			{
			case BUFFER_TO_DEPTH_OR_STENCIL:
				vkCmdCopyBufferToImage(buffer, data.src.buffer, data.dst_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					data.region_count, static_cast<VkBufferImageCopy*>(data.regions));
				break;
			case FULL_CONVERSION:
				vkCmdBlitImage(buffer, data.src.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
					data.dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					data.region_count, static_cast<VkImageBlit*>(data.regions), data.filter);
				break;
			case MULTISAMPLED_TO_SIMPLE:
				vkCmdResolveImage(buffer, data.src.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
					data.dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					data.region_count, static_cast<VkImageResolve*>(data.regions));
				break;
			case MULTISAMPLED_TO_MULTISAMPLED:
				INTERNAL_ASSERT(false, "Unimplemented");
				break;
			default:
				INTERNAL_ASSERT(false, "Invalid conversion");
				break;
			}

			std::free(data.regions);
		}

		m_pending_copies_transfer.clear();
		m_pending_size = 0;
	}
}
