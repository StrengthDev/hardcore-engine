#include <pch.hpp>

#include <render/device.hpp>
#include <render/graphics_pipeline.hpp>

#include <debug/log_internal.hpp>

//pipelines only need to account for the first 2 descriptor sets (0 and 1), as the rest are bound outside the pipeline scope
const std::uint32_t pipeline_descriptor_sets = 2;
const std::uint32_t initial_object_descriptor_set_capacity = 10;
const std::uint32_t object_descriptor_set_increment = 10;

const VkExtent2D placeholder_extent = { 10, 10 };
const VkExtent2D invalid_extent = {
	std::numeric_limits<decltype(VkExtent2D::width)>::max(),
	std::numeric_limits<decltype(VkExtent2D::height)>::max()
};

namespace ENGINE_NAMESPACE
{
	class data_layout_internal : public data_layout
	{
	public:
		data_layout_internal() = delete; //TODO delete this and add accessors, why wouldnt users be able to check the types???

		friend VkFormat to_vk_format(const data_layout::value&);
		friend VkPipelineVertexInputStateCreateInfo to_pipeline_inputs(const std::vector<data_layout>&);
	};

	inline VkFormat to_vk_format(const data_layout::value& type)
	{
		typedef data_layout::component_type component_t;
		typedef data_layout::type vector_t;

		VkFormat format = VK_FORMAT_UNDEFINED;
		switch (type.t)
		{
		case data_layout::type::SCALAR:
			switch (type.ct)
			{
				case component_t::FLOAT16:	format = VK_FORMAT_R16_SFLOAT; break;
				case component_t::FLOAT32:	format = VK_FORMAT_R32_SFLOAT; break;
				case component_t::FLOAT64:	format = VK_FORMAT_R64_SFLOAT; break;
				case component_t::INT8:		format = VK_FORMAT_R8_SINT; break;
				case component_t::INT16:	format = VK_FORMAT_R16_SINT; break;
				case component_t::INT32:	format = VK_FORMAT_R32_SINT; break;
				case component_t::INT64:	format = VK_FORMAT_R64_SINT; break;
				case component_t::UINT8:	format = VK_FORMAT_R8_UINT; break;
				case component_t::UINT16:	format = VK_FORMAT_R16_UINT; break;
				case component_t::UINT32:	format = VK_FORMAT_R32_UINT; break;
				case component_t::UINT64:	format = VK_FORMAT_R64_UINT; break;
			default:
				break;
			}
			break;
		case vector_t::VEC2:
		case vector_t::MAT2:
		case vector_t::MAT3x2:
		case vector_t::MAT4x2:
			switch (type.ct)
			{
				case component_t::FLOAT16:	format = VK_FORMAT_R16G16_SFLOAT; break;
				case component_t::FLOAT32:	format = VK_FORMAT_R32G32_SFLOAT; break;
				case component_t::FLOAT64:	format = VK_FORMAT_R64G64_SFLOAT; break;
				case component_t::INT8:		format = VK_FORMAT_R8G8_SINT; break;
				case component_t::INT16:	format = VK_FORMAT_R16G16_SINT; break;
				case component_t::INT32:	format = VK_FORMAT_R32G32_SINT; break;
				case component_t::INT64:	format = VK_FORMAT_R64G64_SINT; break;
				case component_t::UINT8:	format = VK_FORMAT_R8G8_UINT; break;
				case component_t::UINT16:	format = VK_FORMAT_R16G16_UINT; break;
				case component_t::UINT32:	format = VK_FORMAT_R32G32_UINT; break;
				case component_t::UINT64:	format = VK_FORMAT_R64G64_UINT; break;
			default:
				break;
			}
			break;
		case vector_t::VEC3:
		case vector_t::MAT2x3:
		case vector_t::MAT3:
		case vector_t::MAT4x3:
			switch (type.ct)
			{
				case component_t::FLOAT16:	format = VK_FORMAT_R16G16B16_SFLOAT; break;
				case component_t::FLOAT32:	format = VK_FORMAT_R32G32B32_SFLOAT; break;
				case component_t::FLOAT64:	format = VK_FORMAT_R64G64B64_SFLOAT; break;
				case component_t::INT8:		format = VK_FORMAT_R8G8B8_SINT; break;
				case component_t::INT16:	format = VK_FORMAT_R16G16B16_SINT; break;
				case component_t::INT32:	format = VK_FORMAT_R32G32B32_SINT; break;
				case component_t::INT64:	format = VK_FORMAT_R64G64B64_SINT; break;
				case component_t::UINT8:	format = VK_FORMAT_R8G8B8_UINT; break;
				case component_t::UINT16:	format = VK_FORMAT_R16G16B16_UINT; break;
				case component_t::UINT32:	format = VK_FORMAT_R32G32B32_UINT; break;
				case component_t::UINT64:	format = VK_FORMAT_R64G64B64_UINT; break;
			default:
				break;
			}
			break;
		case vector_t::VEC4:
		case vector_t::MAT2x4:
		case vector_t::MAT3x4:
		case vector_t::MAT4:
			switch (type.ct)
			{
				case component_t::FLOAT16:	format = VK_FORMAT_R16G16B16A16_SFLOAT; break;
				case component_t::FLOAT32:	format = VK_FORMAT_R32G32B32A32_SFLOAT; break;
				case component_t::FLOAT64:	format = VK_FORMAT_R64G64B64A64_SFLOAT; break;
				case component_t::INT8:		format = VK_FORMAT_R8G8B8A8_SINT; break;
				case component_t::INT16:	format = VK_FORMAT_R16G16B16A16_SINT; break;
				case component_t::INT32:	format = VK_FORMAT_R32G32B32A32_SINT; break;
				case component_t::INT64:	format = VK_FORMAT_R64G64B64A64_SINT; break;
				case component_t::UINT8:	format = VK_FORMAT_R8G8B8A8_UINT; break;
				case component_t::UINT16:	format = VK_FORMAT_R16G16B16A16_UINT; break;
				case component_t::UINT32:	format = VK_FORMAT_R32G32B32A32_UINT; break;
				case component_t::UINT64:	format = VK_FORMAT_R64G64B64A64_UINT; break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		return format;
	}

	inline VkPipelineVertexInputStateCreateInfo to_pipeline_inputs(const std::vector<data_layout>& layouts)
	{
		VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		vertex_input_info.vertexBindingDescriptionCount = layouts.size();
		std::uint32_t attribute_description_count = 0;
		VkVertexInputBindingDescription* binding_descriptions = t_calloc<VkVertexInputBindingDescription>(layouts.size());
		for (std::uint32_t i = 0; i < layouts.size(); i++)
		{
			VkVertexInputBindingDescription desc = {};
			binding_descriptions[i].binding = 0;
			binding_descriptions[i].stride = static_cast<std::uint32_t>(layouts[i].size());
			binding_descriptions[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			
			attribute_description_count += layouts[i].vector_count();
		}
		vertex_input_info.pVertexBindingDescriptions = binding_descriptions;
		vertex_input_info.vertexAttributeDescriptionCount = attribute_description_count;
		VkVertexInputAttributeDescription* attribute_descriptions = 
			t_calloc<VkVertexInputAttributeDescription>(attribute_description_count);

		std::uint32_t c = 0;
		for (std::uint32_t i = 0; i < layouts.size(); i++)
		{
			std::uint32_t offset = 0;
			const data_layout_internal& layout = static_cast<const data_layout_internal&>(layouts[i]);
			for (std::uint32_t k = 0; k < layouts[i].vector_count(); k++)
			{
				std::uint32_t iterations; //matrix inputs are passed as several vectors
				switch (layout[k].t)
				{
				case data_layout::type::MAT2:
				case data_layout::type::MAT2x3:
				case data_layout::type::MAT2x4:
					iterations = 2;
					break;
				case data_layout::type::MAT3:
				case data_layout::type::MAT3x2:
				case data_layout::type::MAT3x4:
					iterations = 3;
					break;
				case data_layout::type::MAT4:
				case data_layout::type::MAT4x2:
				case data_layout::type::MAT4x3:
					iterations = 4;
					break;
				default:
					iterations = 1;
					break;
				}

				const VkFormat format = to_vk_format(layout[k]);
				INTERNAL_ASSERT(format != VK_FORMAT_UNDEFINED, "Invalid type.");
				for (std::uint32_t j = 0; j < iterations; j++)
				{
					attribute_descriptions[c].binding = i;
					attribute_descriptions[c].location = k + j;
					attribute_descriptions[c].format = format;
					attribute_descriptions[c].offset = offset;

					offset += layout[k].size() / iterations;
					c++;
				}
			}
		}
		vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions;

		return vertex_input_info;
	}

	inline VkDescriptorType to_vk_descriptor_type(shader::descriptor_t type)
	{
		switch (type)
		{
		case ENGINE_NAMESPACE::shader::UNIFORM:			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case ENGINE_NAMESPACE::shader::STORAGE:			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case ENGINE_NAMESPACE::shader::SAMPLER:			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case ENGINE_NAMESPACE::shader::TEXTURE:			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case ENGINE_NAMESPACE::shader::IMAGE:			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case ENGINE_NAMESPACE::shader::SAMPLER_BUFFER:	return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		case ENGINE_NAMESPACE::shader::IMAGE_BUFFER:	return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		case ENGINE_NAMESPACE::shader::SAMPLER_SHADOW:	return VK_DESCRIPTOR_TYPE_SAMPLER; 
		case ENGINE_NAMESPACE::shader::NONE:
		default:
			INTERNAL_ASSERT(false, "Invalid shader variable type");
			break;
		}
		return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}

	inline void alloc_descriptor_sets(VkDevice& handle, std::uint32_t object_descriptor_capacity,
		const std::uint32_t* n_pool_sizes, VkDescriptorPoolSize* pool_sizes[pipeline_descriptor_sets], 
		const VkDescriptorSetLayout* descriptor_set_layouts, VkDescriptorPool* out_descriptor_pool, VkDescriptorSet** out_descriptor_sets)
	{
		std::unordered_map<VkDescriptorType, VkDescriptorPoolSize> pool_sizes_map;
		for (std::uint32_t i = 0; i < n_pool_sizes[0]; i++)
			pool_sizes_map[pool_sizes[0][i].type].descriptorCount += pool_sizes[0][i].descriptorCount * object_descriptor_capacity;
		for (std::uint32_t i = 0; i < n_pool_sizes[1]; i++)
			pool_sizes_map[pool_sizes[1][i].type].descriptorCount += pool_sizes[1][i].descriptorCount;

		std::uint32_t sizes_count = 0;
		VkDescriptorPoolSize* pool_sizes_ptr = t_malloc<VkDescriptorPoolSize>(pool_sizes_map.size());
		for (auto& pool_size : pool_sizes_map)
		{
			pool_sizes_ptr[sizes_count] = pool_size.second;
			pool_sizes_ptr[sizes_count].type = pool_size.first;
			pool_sizes_ptr[sizes_count].descriptorCount *= max_frames_in_flight;
			sizes_count++;
		}

		std::uint32_t pipeline_descriptor = n_pool_sizes[1] ? 1 : 0;
		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.poolSizeCount = sizes_count;
		pool_info.pPoolSizes = pool_sizes_ptr;
		pool_info.maxSets = (object_descriptor_capacity + pipeline_descriptor) * max_frames_in_flight;
		pool_info.flags = 0;// VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		VK_CRASH_CHECK(vkCreateDescriptorPool(handle, &pool_info, nullptr, out_descriptor_pool), "Failed to create descriptor pool");
		std::free(pool_sizes_ptr);

		VkDescriptorSetLayout* layouts = t_malloc<VkDescriptorSetLayout>(pool_info.maxSets);
		if (n_pool_sizes[0])
			for (std::uint32_t f = 0; f < max_frames_in_flight; f++)
				for (std::uint32_t i = 0; i < object_descriptor_capacity; i++)
					layouts[i + pipeline_descriptor + (pipeline_descriptor + object_descriptor_capacity) * f] = descriptor_set_layouts[0];
		if (n_pool_sizes[1])
			for (std::uint32_t f = 0; f < max_frames_in_flight; f++)
				layouts[(pipeline_descriptor + object_descriptor_capacity) * f] = descriptor_set_layouts[1];

		*out_descriptor_sets = t_malloc<VkDescriptorSet>(pool_info.maxSets);
		VkDescriptorSetAllocateInfo set_alloc_info = {};
		set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		set_alloc_info.descriptorPool = *out_descriptor_pool;
		set_alloc_info.descriptorSetCount = pool_info.maxSets;
		set_alloc_info.pSetLayouts = layouts;

		VK_CRASH_CHECK(vkAllocateDescriptorSets(handle, &set_alloc_info, *out_descriptor_sets), "Failed to allocate descriptor sets");
		std::free(layouts);
	}

	inline void init_descriptors(VkDevice& handle, const std::vector<const shader*>& shaders,
		std::uint32_t* n_pool_sizes, VkDescriptorPoolSize* pool_sizes[pipeline_descriptor_sets],
		std::uint32_t* out_n_descriptor_set_layouts, VkDescriptorSetLayout** out_descriptor_set_layouts,
		VkDescriptorPool* out_descriptor_pool, VkDescriptorSet** out_descriptor_sets,
		std::uint32_t* out_n_object_descriptors, VkDescriptorType** out_object_descriptors)
	{
		std::vector<std::vector<VkDescriptorSetLayoutBinding>> sets;
		std::unordered_map<VkDescriptorType, VkDescriptorPoolSize> pool_sizes_0;
		std::unordered_map<VkDescriptorType, VkDescriptorPoolSize> pool_sizes_1;

		//parse bindings
		for (const shader* p : shaders)
		{
			const internal_shader& cast_shader = *static_cast<const internal_shader*>(p);
			const std::vector<shader::descriptor_data>& descriptors = cast_shader.descriptors();

			for (const auto& descriptor : descriptors)
			{
				const VkDescriptorSetLayoutBinding null_binding = { 0, VK_DESCRIPTOR_TYPE_MAX_ENUM, 0, 0, nullptr };

				const std::size_t min_set_size = static_cast<std::size_t>(descriptor.set + 1);
				if (sets.size() < min_set_size) sets.resize(min_set_size);

				const std::size_t min_binding_size = static_cast<std::size_t>(descriptor.binding + 1);
				if (sets[descriptor.set].size() < min_binding_size)
					sets[descriptor.set].resize(min_binding_size, null_binding);

				VkDescriptorSetLayoutBinding& layout_binding = sets[descriptor.set][descriptor.binding];
				VkDescriptorType descriptor_type = to_vk_descriptor_type(descriptor.type);
				if (layout_binding.descriptorType != VK_DESCRIPTOR_TYPE_MAX_ENUM)
				{
					if (layout_binding.descriptorType == descriptor_type && layout_binding.descriptorCount == descriptor.count)
						layout_binding.stageFlags |= cast_shader.get_stage();
					else INTERNAL_ASSERT(false, "Different descriptor for the same binding in different shaders"); //TODO throw exception instead
				}
				else
				{
					const std::uint32_t count = descriptor.count ? descriptor.count : 1;

					layout_binding.binding = descriptor.binding;
					layout_binding.descriptorCount = count;
					layout_binding.stageFlags = cast_shader.get_stage();
					layout_binding.pImmutableSamplers = nullptr; //TODO deal with this
					layout_binding.descriptorType = descriptor_type;

					if (descriptor.set == 0)
						pool_sizes_0[descriptor_type].descriptorCount += count;
					else
					{
						if (descriptor.set == 1) pool_sizes_1[descriptor_type].descriptorCount += count;
					}
				}
			}
		}

		for (auto& set : sets)
			set.erase(std::remove_if(set.begin(), set.end(), 
				[](const VkDescriptorSetLayoutBinding& b) { return b.descriptorType == VK_DESCRIPTOR_TYPE_MAX_ENUM; }), set.end());

		if(sets.size())
			for (auto& binding : sets[0])
			{
				if (binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
					binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				else if (binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
					binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			}

		//init descriptor set layouts
		*out_n_descriptor_set_layouts = 0;
		*out_descriptor_set_layouts = t_malloc<VkDescriptorSetLayout>(sets.size());
		std::uint32_t total_pipeline_sets = 0;
		for (auto& set : sets)
		{
			if (set.empty()) (*out_descriptor_set_layouts)[*out_n_descriptor_set_layouts] = VK_NULL_HANDLE;
			else
			{
				VkDescriptorSetLayoutCreateInfo layout_info = {};
				layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layout_info.bindingCount = static_cast<std::uint32_t>(set.size());
				layout_info.pBindings = set.data();

				VK_CRASH_CHECK(vkCreateDescriptorSetLayout(handle, &layout_info, nullptr, 
					&(*out_descriptor_set_layouts)[*out_n_descriptor_set_layouts]), "Failed to create descriptor set layout");

				if (*out_n_descriptor_set_layouts < pipeline_descriptor_sets) total_pipeline_sets++;
			}
			(*out_n_descriptor_set_layouts)++;
		}

		//init descriptor sets
		if (total_pipeline_sets)
		{
			std::uint32_t sizes_count = 0;
			n_pool_sizes[0] = static_cast<std::uint32_t>(pool_sizes_0.size());
			pool_sizes[0] = t_malloc<VkDescriptorPoolSize>(n_pool_sizes[0]);
			for (auto& pool_size : pool_sizes_0)
			{
				(pool_sizes[0])[sizes_count] = pool_size.second;
				(pool_sizes[0])[sizes_count].type = pool_size.first;
				sizes_count++;
			}

			if (sizes_count)
			{
				*out_n_object_descriptors = static_cast<std::uint32_t>(sets[0].size());
				*out_object_descriptors = t_malloc<VkDescriptorType>(*out_n_object_descriptors);
				for (std::uint32_t i = 0; i < *out_n_object_descriptors; i++) (*out_object_descriptors)[i] = sets[0][i].descriptorType;
			}

			sizes_count = 0;
			n_pool_sizes[1] = static_cast<std::uint32_t>(pool_sizes_1.size());
			pool_sizes[1] = t_malloc<VkDescriptorPoolSize>(n_pool_sizes[1]);
			for (auto& pool_size : pool_sizes_1)
			{
				(pool_sizes[1])[sizes_count] = pool_size.second;
				(pool_sizes[1])[sizes_count].type = pool_size.first;
				sizes_count++;
			}

			alloc_descriptor_sets(handle, initial_object_descriptor_set_capacity, n_pool_sizes, pool_sizes,
				*out_descriptor_set_layouts, out_descriptor_pool, out_descriptor_sets);
		}
	}

	graphics_pipeline::graphics_pipeline(device& owner, const std::vector<const shader*>& shaders)
		: graphics_pipeline(owner, shaders, placeholder_extent, true) {}

	graphics_pipeline::graphics_pipeline(device& owner, const std::vector<const shader*>& shaders,
		const VkExtent2D& extent) : graphics_pipeline(owner, shaders, extent, false) {}

	graphics_pipeline::graphics_pipeline(device& owner, const std::vector<const shader*>& shaders, 
		const VkExtent2D& extent, bool dynamic_viewport) : owner(&owner)
	{
		LOG_INTERNAL_INFO("Initialising new graphics pipeline..");

		VkPipelineVertexInputStateCreateInfo vertex_input_info = 
			to_pipeline_inputs(static_cast<const internal_shader*>(shaders[0])->inputs());

		init_descriptors(owner.handle, shaders, n_descriptor_pool_sizes, descriptor_pool_sizes,
			&n_descriptor_set_layouts, &descriptor_set_layouts, &frame_descriptors[0].descriptor_pool, &descriptor_sets,
			&n_object_bindings, &object_binding_types);

		if (n_descriptor_pool_sizes[0])
		{
			object_descriptor_set_capacity = initial_object_descriptor_set_capacity;
			n_dynamic_descriptors = 0;
			for (std::uint32_t i = 0; i < n_object_bindings; i++)
				if (object_binding_types[i] == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
					object_binding_types[i] == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
					n_dynamic_descriptors++;
		}
		frame_descriptor_count = object_descriptor_set_capacity + (n_descriptor_pool_sizes[1] ? 1 : 0);
		for (std::uint8_t i = 1; i < max_frames_in_flight; i++)
		{
			frame_descriptors[i].descriptor_pool = frame_descriptors[0].descriptor_pool;
			frame_descriptors[i].object_set_cap = frame_descriptors[0].object_set_cap;
		}

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; //primitives
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(extent.width);
		viewport.height = static_cast<float>(extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f; //always between 1 and 0

		VkRect2D scissor = {}; //all values measured in pixels
		scissor.offset = { 0, 0 };
		scissor.extent = extent;

		VkPipelineViewportStateCreateInfo viewport_state = {};
		viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state.viewportCount = 1;
		viewport_state.pViewports = &viewport;
		viewport_state.scissorCount = 1;
		viewport_state.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE; //requires enabling gpu feature
		rasterizer.rasterizerDiscardEnable = VK_FALSE; //idk why this would be true
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //rendermode: fill, line, point - requires enabling gpu feature when not fill
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; //face culling, front, back or both
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		VkPipelineMultisampleStateCreateInfo multisampling = {}; //requires enabling gpu feature
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		//DepthStencil missing

		VkPipelineColorBlendAttachmentState color_blend_attachment = {};
		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable = VK_FALSE;
		color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		/* regular alpha blending
		color_blend_attachment.blendEnable = VK_TRUE;
		color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
		*/

		VkPipelineColorBlendStateCreateInfo color_blending = {};
		color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blending.logicOpEnable = VK_FALSE;
		color_blending.logicOp = VK_LOGIC_OP_COPY; // Optional
		color_blending.attachmentCount = 1;
		color_blending.pAttachments = &color_blend_attachment;
		color_blending.blendConstants[0] = 0.0f; // Optional
		color_blending.blendConstants[1] = 0.0f; // Optional
		color_blending.blendConstants[2] = 0.0f; // Optional
		color_blending.blendConstants[3] = 0.0f; // Optional

		VkPipelineDynamicStateCreateInfo dynamic_state = {};
		dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state.pNext = nullptr;
		dynamic_state.flags = 0;
		dynamic_state.dynamicStateCount = 2;
		VkDynamicState states_array[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		dynamic_state.pDynamicStates = states_array;

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = n_descriptor_set_layouts;
		pipeline_layout_info.pSetLayouts = descriptor_set_layouts;
		pipeline_layout_info.pushConstantRangeCount = 0;
		pipeline_layout_info.pPushConstantRanges = nullptr;

		VK_CRASH_CHECK(vkCreatePipelineLayout(owner.handle, &pipeline_layout_info, nullptr, &pipeline_layout),
			"Failed to create pipeline layout");

		std::uint32_t stage_count = 0;
		VkPipelineShaderStageCreateInfo* shader_stages = t_calloc<VkPipelineShaderStageCreateInfo>(shaders.size());
		for (const shader* shader : shaders)
		{
			shader_stages[stage_count].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_stages[stage_count].module = internal_shader::create_shader_module(*shader, owner.handle, 
				&shader_stages[stage_count].stage, &shader_stages[stage_count].pName);
			stage_count++;
		}

		VkGraphicsPipelineCreateInfo pipeline_info = {};
		pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.stageCount = stage_count;
		pipeline_info.pStages = shader_stages;
		pipeline_info.pVertexInputState = &vertex_input_info;
		pipeline_info.pInputAssemblyState = &inputAssembly;
		pipeline_info.pViewportState = &viewport_state;
		pipeline_info.pRasterizationState = &rasterizer;
		pipeline_info.pMultisampleState = &multisampling;
		pipeline_info.pDepthStencilState = nullptr; // Optional
		pipeline_info.pColorBlendState = &color_blending;
		pipeline_info.pDynamicState = dynamic_viewport ? &dynamic_state : nullptr;
		pipeline_info.layout = pipeline_layout;
		pipeline_info.renderPass = owner.render_pass;
		pipeline_info.subpass = 0;
		pipeline_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipeline_info.basePipelineIndex = -1; // Optional
		pipeline_info.pNext = nullptr;
		pipeline_info.flags = 0;
		pipeline_info.pTessellationState = nullptr;

		VK_CRASH_CHECK(vkCreateGraphicsPipelines(owner.handle, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &handle),
			"Failed to create pipeline");

		objects = object_vector(0, n_object_bindings, n_dynamic_descriptors);

		for (std::uint32_t i = 0; i < stage_count; i++)
		{
			vkDestroyShaderModule(owner.handle, shader_stages[i].module, nullptr);
			std::free(const_cast<char*>(shader_stages[i].pName)); //a bit messy, but shouldn't cause issues
		}

		std::free(shader_stages);

		//ideally these shouldnt be manually allocated
		std::free(const_cast<VkVertexInputBindingDescription*>(vertex_input_info.pVertexBindingDescriptions)); //a bit messy, but shouldn't cause issues
		std::free(const_cast<VkVertexInputAttributeDescription*>(vertex_input_info.pVertexAttributeDescriptions)); //a bit messy, but shouldn't cause issues
	}

	graphics_pipeline::~graphics_pipeline()
	{
		if (handle != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(owner->handle, handle, nullptr);
			vkDestroyPipelineLayout(owner->handle, pipeline_layout, nullptr);

			VkDescriptorPool t_pool = VK_NULL_HANDLE;
			for (std::uint8_t i = 0; i < max_frames_in_flight; i++)
			{
				if (frame_descriptors[i].descriptor_pool != t_pool)
				{
					t_pool = frame_descriptors[i].descriptor_pool;
					vkDestroyDescriptorPool(owner->handle, frame_descriptors[i].descriptor_pool, nullptr);
				}
			}

			for (std::uint32_t i = 0; i < n_descriptor_set_layouts; i++)
				if (descriptor_set_layouts[i] != VK_NULL_HANDLE)
					vkDestroyDescriptorSetLayout(owner->handle, descriptor_set_layouts[i], nullptr);

			std::free(descriptor_set_layouts);
			std::free(descriptor_pool_sizes[0]);
			std::free(descriptor_pool_sizes[1]);
			std::free(descriptor_sets);
			std::free(object_binding_types);
		}
	}

	inline VkIndexType to_vk_index_type(mesh::index_format format)
	{
		switch (format)
		{
		case mesh::index_format::NONE:
			return VK_INDEX_TYPE_NONE_KHR;
		case mesh::index_format::UINT8:
			return VK_INDEX_TYPE_UINT8_EXT;
		case mesh::index_format::UINT16:
			return VK_INDEX_TYPE_UINT16;
		case mesh::index_format::UINT32:
			return VK_INDEX_TYPE_UINT32;
		default:
			INTERNAL_ASSERT(false, "Index format not implemented");
		}
		return VK_INDEX_TYPE_NONE_KHR;
	}

	inline void graphics_pipeline::draw_object(VkCommandBuffer& buffer, const task_properties& obj)
	{
		vkCmdBindVertexBuffers(buffer, 0, 1, &obj.binding.buffer, &obj.binding.offset);
		if (obj.index_t == VK_INDEX_TYPE_NONE_KHR)
		{
			vkCmdDraw(buffer, obj.count, obj.instances, 0, 0);
		}
		else
		{
			vkCmdBindIndexBuffer(buffer, obj.index_binding.buffer, obj.index_binding.offset, obj.index_t);
			vkCmdDrawIndexed(buffer, obj.count, obj.instances, 0, 0, 0);
		}
	}

	void graphics_pipeline::record_commands(VkCommandBuffer& buffer, std::uint8_t current_frame)
	{
		vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, handle);

		//pipeline sets
		if (n_descriptor_pool_sizes[1])
			vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
				&descriptor_sets[frame_descriptor_count * current_frame], 0, nullptr);

		if (n_descriptor_pool_sizes[0])
		{
			if (push_data_size)
			{
				for (const auto& obj : objects)
				{
					vkCmdPushConstants(buffer, pipeline_layout, push_flags, 0, push_data_size, obj.push_data());

					vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, 
						&descriptor_sets[obj.properties().descriptor_set_idx + frame_descriptor_count * current_frame], 
						n_dynamic_descriptors, obj.dynamic_offsets());
				
					draw_object(buffer, obj.properties());
				}
			}
			else
			{
				for (const auto& obj : objects)
				{
					vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
						&descriptor_sets[obj.properties().descriptor_set_idx + frame_descriptor_count * current_frame], 
						n_dynamic_descriptors, obj.dynamic_offsets());

					draw_object(buffer, obj.properties());
				}
			}
		}
		else
		{
			if (push_data_size)
			{
				for (const auto& obj : objects)
				{
					vkCmdPushConstants(buffer, pipeline_layout, push_flags, 0, push_data_size, obj.push_data());

					draw_object(buffer, obj.properties());
				}
			}
			else
			{
				for (const auto& obj : objects)
					draw_object(buffer, obj.properties());
			}
		}
	}

	inline std::vector<VkWriteDescriptorSet> graphics_pipeline::generate_descriptor_write(std::uint8_t current_frame)
	{
		const std::uint32_t pipeline_descriptor = n_descriptor_pool_sizes[1] ? 1 : 0;
		cached_buffer_infos.clear();
		cached_buffer_infos.reserve(objects.size() * n_dynamic_descriptors); // must reserve here to avoid resizes
		std::vector<VkWriteDescriptorSet> res;
		res.reserve(cached_object_bindings.size());
		for (std::size_t i = 0; i < cached_object_bindings.size(); i++)
		{
			const buffer_binding_args& buffer = cached_object_bindings[i];
			const std::size_t set = i / n_object_bindings;
			const std::size_t binding = i % n_object_bindings;

			res.push_back({});

			VkWriteDescriptorSet& descriptor_write = res[i];
			descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_write.dstSet = descriptor_sets[pipeline_descriptor + set + frame_descriptor_count * current_frame]; //TODO chanage
			descriptor_write.dstBinding = binding;
			descriptor_write.dstArrayElement = 0;
			descriptor_write.descriptorType = object_binding_types[binding];
			descriptor_write.descriptorCount = 1;
			descriptor_write.pBufferInfo = nullptr;
			descriptor_write.pImageInfo = nullptr;
			descriptor_write.pTexelBufferView = nullptr;
			
			switch (object_binding_types[binding])
			{
			case VK_DESCRIPTOR_TYPE_SAMPLER:
				INTERNAL_ASSERT(false, "Unimplemented");
				break;
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				INTERNAL_ASSERT(false, "Unimplemented");
				break;
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				INTERNAL_ASSERT(false, "Unimplemented");
				break;
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				INTERNAL_ASSERT(false, "Unimplemented");
				break;
			case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
				INTERNAL_ASSERT(false, "Unimplemented");
				break;
			case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
				INTERNAL_ASSERT(false, "Unimplemented");
				break;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			{
				const std::size_t buffer_idx = cached_buffer_infos.size();
				cached_buffer_infos.push_back({});
				VkDescriptorBufferInfo& buffer_info = cached_buffer_infos[buffer_idx];
				buffer_info.buffer = buffer.buffer;
				buffer_info.offset = buffer.frame_offset * current_frame;
				buffer_info.range = buffer.size;
				descriptor_write.pBufferInfo = &buffer_info;
			}
				break;
			case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
				INTERNAL_ASSERT(false, "Unimplemented");
				break;
			case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT:
				INTERNAL_ASSERT(false, "Unimplemented");
				break;
			case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
				INTERNAL_ASSERT(false, "Unimplemented");
				break;
			case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
				INTERNAL_ASSERT(false, "Unimplemented");
				break;
			case VK_DESCRIPTOR_TYPE_MUTABLE_VALVE:
				INTERNAL_ASSERT(false, "Unimplemented");
				break;
			default:
				INTERNAL_ASSERT(false, "Invalid descriptor type");
				break;
			}
		}
		return res;
	}

	void graphics_pipeline::update_descriptor_sets(std::uint8_t previous_frame, std::uint8_t current_frame, std::uint8_t next_frame)
	{
		if (!n_descriptor_pool_sizes[0]) return;

		if (frame_descriptors[current_frame].object_set_cap < frame_descriptors[previous_frame].object_set_cap)
		{
			if (frame_descriptors[current_frame].descriptor_pool != frame_descriptors[next_frame].descriptor_pool)
				vkDestroyDescriptorPool(owner->handle, frame_descriptors[current_frame].descriptor_pool, nullptr);
				
			frame_descriptors[current_frame].descriptor_pool = frame_descriptors[previous_frame].descriptor_pool;
			frame_descriptors[current_frame].object_set_cap = frame_descriptors[previous_frame].object_set_cap;
		}

		if (frame_descriptors[current_frame].dirty)
		{
			for (std::uint8_t i = 0; i < current_frame; i++)
				frame_descriptors[i].outdated = true;
			for (std::uint8_t i = current_frame + 1; i < max_frames_in_flight; i++)
				frame_descriptors[i].outdated = true;

			cached_object_bindings.clear();

			for (const auto& obj : objects)
			{
				buffer_binding_args* bindings = obj.bindings();
				for (std::uint32_t i = 0; i < n_object_bindings; i++)
					cached_object_bindings.push_back(bindings[i]);
			}

			std::vector<std::uint32_t> duplicate_ranges;
			std::uint32_t i = 0, j = 1, unique_set_counter = 0, first_of_duplicates = 1;
			bool not_sorted = objects.size();

			while (not_sorted)
			{
				if (i >= objects.size()) break;
				objects[i].properties().descriptor_set_idx = unique_set_counter;
				
				while (j - i < 2)
				{
					if (j < objects.size())
					{
						bool equal_set = true;
						for (std::uint32_t a = i * n_object_bindings, b = j * n_object_bindings; b < (j + 1) * n_object_bindings; a++, b++)
						{
							if (cached_object_bindings[a].buffer != cached_object_bindings[b].buffer)
							{
								equal_set = false;
								break;
							}
						}

						if (equal_set)
						{
							objects[j].properties().descriptor_set_idx = unique_set_counter; //TODO add  pipeline scope descriptor count
							i++;
						}

						j++;
					}
					else
					{
						not_sorted = false;
						break;
					}
				}

				while (j < objects.size())
				{
					bool equal_set = true;
					for (std::uint32_t a = i * n_object_bindings, b = j * n_object_bindings; b < (j + 1) * n_object_bindings; a++, b++)
					{
						if (cached_object_bindings[a].buffer != cached_object_bindings[b].buffer)
						{
							equal_set = false;
							break;
						}
					}

					if (equal_set)
					{
						objects[j].properties().descriptor_set_idx = unique_set_counter;
						i++;
						objects.swap(i, j);
						for (std::uint32_t a = i * n_object_bindings, b = j * n_object_bindings; b < (j + 1) * n_object_bindings; a++, b++)
						{
							buffer_binding_args t = cached_object_bindings[a];
							cached_object_bindings[a] = cached_object_bindings[b];
							cached_object_bindings[b] = t;
						}
					}

					j++;
				}

				unique_set_counter++;
				i++;
				if (first_of_duplicates != i)
				{
					duplicate_ranges.push_back(first_of_duplicates);
					duplicate_ranges.push_back(i);
					first_of_duplicates = i;
				}
				j = i + 1;
			}

			std::uint32_t total_erased = 0;
			for (i = 0; i < duplicate_ranges.size(); i += 2)
			{
				cached_object_bindings.erase(cached_object_bindings.begin() + (duplicate_ranges[i] - total_erased) * n_object_bindings,
					cached_object_bindings.begin() + (duplicate_ranges[i + 2] - total_erased) * n_object_bindings);
				total_erased += duplicate_ranges[i + 1] - duplicate_ranges[i];
			}

			if (frame_descriptors[current_frame].object_set_cap < cached_object_bindings.size() / n_object_bindings)
			{
				std::free(descriptor_sets);
				if (max_frames_in_flight == 1)
					vkDestroyDescriptorPool(owner->handle, frame_descriptors[current_frame].descriptor_pool, nullptr);

				do //unoptimal
				{
					frame_descriptors[current_frame].object_set_cap += object_descriptor_set_increment;
				} while (frame_descriptors[current_frame].object_set_cap < cached_object_bindings.size() / n_object_bindings);

				alloc_descriptor_sets(owner->handle, frame_descriptors[current_frame].object_set_cap,
					n_descriptor_pool_sizes, descriptor_pool_sizes, descriptor_set_layouts,
					&frame_descriptors[current_frame].descriptor_pool, &descriptor_sets);
			}

			auto descriptor_write = generate_descriptor_write(current_frame);
			vkUpdateDescriptorSets(owner->handle, descriptor_write.size(), descriptor_write.data(), 0, nullptr);
		}
		else if (frame_descriptors[current_frame].outdated)
		{
			auto descriptor_write = generate_descriptor_write(current_frame);
			vkUpdateDescriptorSets(owner->handle, descriptor_write.size(), descriptor_write.data(), 0, nullptr);
		}

		frame_descriptors[current_frame].dirty = false;
		frame_descriptors[current_frame].outdated = false;
	}

	std::size_t graphics_pipeline::add(const mesh& mesh)
	{
		std::size_t key = std::hash<resource>{}(mesh);
		INTERNAL_ASSERT(!object_refs.count(key), "Object already attached to pipeline");

		frame_descriptors[owner->current_frame].dirty = true;
		std::size_t idx = objects.size();
		auto obj = objects.add();
		auto& props = obj.properties();
		props.binding = owner->get_memory().binding_args(mesh);
		props.count = mesh.draw_count();
		props.index_t = to_vk_index_type(mesh.index_type());
		if (props.index_t != VK_INDEX_TYPE_NONE_KHR)
			props.index_binding = owner->get_memory().index_binding_args(mesh);
		props.instances = 1;
		props.ref_key = key;
		object_refs[props.ref_key] = idx;
		return idx;
	}

	void graphics_pipeline::remove(const mesh& mesh)
	{
		frame_descriptors[owner->current_frame].dirty = true;
		std::size_t key = std::hash<resource>{}(mesh);
		std::size_t idx = object_refs[key];
		object_refs.erase(key);
		objects.remove(idx);

		for (std::size_t i = idx; i < objects.size(); i++)
			object_refs[objects[i].properties().ref_key]--;
	}

	std::size_t graphics_pipeline::set_instances(const mesh& object, std::uint32_t num)
	{
		auto it = object_refs.find(std::hash<resource>{}(object));
		if (it == object_refs.end())
			throw std::range_error("Specified mesh not found in pipeline");
		std::size_t idx = it->second;
		set_instances(idx, num);
		return idx;
	}

	buffer_binding_args graphics_pipeline::get_binding(const uniform& resource) const 
	{ return owner->get_memory().binding_args(resource); }
	buffer_binding_args graphics_pipeline::get_binding(const unmapped_uniform& resource) const
	{ return owner->get_memory().binding_args(resource); }
	buffer_binding_args graphics_pipeline::get_binding(const storage_array& resource) const
	{ return owner->get_memory().binding_args(resource); }
	buffer_binding_args graphics_pipeline::get_binding(const dynamic_storage_array& resource) const
	{ return owner->get_memory().binding_args(resource); }
	buffer_binding_args graphics_pipeline::get_binding(const storage_vector& resource) const
	{ return owner->get_memory().binding_args(resource); }
	buffer_binding_args graphics_pipeline::get_binding(const dynamic_storage_vector& resource) const
	{ return owner->get_memory().binding_args(resource); }

	void graphics_pipeline::set_descriptor(std::size_t object_idx, std::uint32_t binding_idx, buffer_binding_args&& binding)
	{
		frame_descriptors[owner->current_frame].dirty = true;
		auto obj = objects[object_idx];
		obj.bindings()[binding_idx] = binding;
		//TODO binding_idx takes textures and other things into account, which must be skipped for the dynamic_offset_idx
		obj.dynamic_offsets()[binding_idx] = static_cast<dynamic_offset_t>(binding.offset);
	}

	graphics_pipeline::object_vector::object_vector(std::size_t push_data_size, std::uint32_t n_descriptors, 
		std::uint32_t n_dynamic_descriptors) :
		count(0), capacity(4), push_data_size(push_data_size), n_descriptors(n_descriptors), n_dynamic_descriptors(n_dynamic_descriptors)
	{
		element_stride = element_base_size + push_data_size + n_dynamic_descriptors * sizeof(offset_t);
		descriptor_stride = n_descriptors * sizeof(buffer_binding_args);
		object_data = ex_malloc(capacity * element_stride);
		if (n_descriptors)
			object_descriptor_data = t_malloc<buffer_binding_args>(capacity * n_descriptors);
		smp = ex_malloc(std::max(element_stride, descriptor_stride));
	}

	graphics_pipeline::object_vector::~object_vector()
	{
		std::free(object_data);
		std::free(object_descriptor_data);
		std::free(smp);
	}

	graphics_pipeline::object_vector::object_ref graphics_pipeline::object_vector::add()
	{
		std::size_t idx = count++;

		if (count >= capacity)
		{
			capacity *= 2;
			object_data = ex_realloc(object_data, capacity * element_stride);
			object_descriptor_data = t_realloc<buffer_binding_args>(object_descriptor_data, capacity * n_descriptors);
		}

		void* object_p = static_cast<std::byte*>(object_data) + idx * element_stride;
		buffer_binding_args* binding_p = object_descriptor_data + idx * n_descriptors;
		std::memset(binding_p, 0, descriptor_stride);
		return object_ref(object_p, binding_p, push_data_size, n_dynamic_descriptors * sizeof(offset_t));
	}

	void graphics_pipeline::object_vector::remove(std::size_t idx)
	{
		//TODO
	}

	void graphics_pipeline::object_vector::swap(std::size_t a, std::size_t b)
	{
		void* aop = static_cast<std::byte*>(object_data) + element_stride * a;
		void* bop = static_cast<std::byte*>(object_data) + element_stride * b;
		void* adp = object_descriptor_data + n_descriptors * a;
		void* bdp = object_descriptor_data + n_descriptors * b;
		
		std::memcpy(smp, aop, element_stride);
		std::memcpy(aop, bop, element_stride);
		std::memcpy(bop, smp, element_stride);
		std::memcpy(smp, adp, descriptor_stride);
		std::memcpy(adp, bdp, descriptor_stride);
		std::memcpy(bdp, smp, descriptor_stride);
	}
}
