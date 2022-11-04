#include <pch.hpp>

#include <spiral/render/device.hpp>
#include <spiral/render/graphics_pipeline.hpp>

//pipelines only need to account for the first 2 descriptor sets (0 and 1), as the rest are bound outside the pipeline scope
const std::uint32_t pipeline_descriptor_sets = 2;
const std::uint32_t initial_object_descriptor_set_capacity = 10;
const std::uint32_t object_descriptor_set_increment = 10;

namespace ENGINE_NAMESPACE
{
	class data_layout_internal : data_layout
	{
	public:
		data_layout_internal() = delete; //TODO delete this and add accessors, why wouldnt users be able to check the types???

		friend VkFormat to_vk_format(const data_layout::value&);
		friend VkPipelineVertexInputStateCreateInfo get_vertex_inputs(const data_layout*, std::uint32_t);
	};

#define CASE(cond, form) case data_layout::component_type::cond: format = form; break;
	inline VkFormat to_vk_format(const data_layout::value& type)
	{
		VkFormat format = VK_FORMAT_UNDEFINED;
		switch (type.t)
		{
		case data_layout::type::SCALAR:
			switch (type.ct)
			{
				CASE(FLOAT16, VK_FORMAT_R16_SFLOAT);
				CASE(FLOAT32, VK_FORMAT_R32_SFLOAT);
				CASE(FLOAT64, VK_FORMAT_R64_SFLOAT);
				CASE(INT8, VK_FORMAT_R8_SINT);
				CASE(INT16, VK_FORMAT_R16_SINT);
				CASE(INT32, VK_FORMAT_R32_SINT);
				CASE(INT64, VK_FORMAT_R64_SINT);
				CASE(UINT8, VK_FORMAT_R8_UINT);
				CASE(UINT16, VK_FORMAT_R16_UINT);
				CASE(UINT32, VK_FORMAT_R32_UINT);
				CASE(UINT64, VK_FORMAT_R64_UINT);
			default:
				break;
			}
			break;
		case data_layout::type::VEC2:
		case data_layout::type::MAT2:
		case data_layout::type::MAT3x2:
		case data_layout::type::MAT4x2:
			switch (type.ct)
			{
				CASE(FLOAT16, VK_FORMAT_R16G16_SFLOAT);
				CASE(FLOAT32, VK_FORMAT_R32G32_SFLOAT);
				CASE(FLOAT64, VK_FORMAT_R64G64_SFLOAT);
				CASE(INT8, VK_FORMAT_R8G8_SINT);
				CASE(INT16, VK_FORMAT_R16G16_SINT);
				CASE(INT32, VK_FORMAT_R32G32_SINT);
				CASE(INT64, VK_FORMAT_R64G64_SINT);
				CASE(UINT8, VK_FORMAT_R8G8_UINT);
				CASE(UINT16, VK_FORMAT_R16G16_UINT);
				CASE(UINT32, VK_FORMAT_R32G32_UINT);
				CASE(UINT64, VK_FORMAT_R64G64_UINT);
			default:
				break;
			}
			break;
		case data_layout::type::VEC3:
		case data_layout::type::MAT2x3:
		case data_layout::type::MAT3:
		case data_layout::type::MAT4x3:
			switch (type.ct)
			{
				CASE(FLOAT16, VK_FORMAT_R16G16B16_SFLOAT);
				CASE(FLOAT32, VK_FORMAT_R32G32B32_SFLOAT);
				CASE(FLOAT64, VK_FORMAT_R64G64B64_SFLOAT);
				CASE(INT8, VK_FORMAT_R8G8B8_SINT);
				CASE(INT16, VK_FORMAT_R16G16B16_SINT);
				CASE(INT32, VK_FORMAT_R32G32B32_SINT);
				CASE(INT64, VK_FORMAT_R64G64B64_SINT);
				CASE(UINT8, VK_FORMAT_R8G8B8_UINT);
				CASE(UINT16, VK_FORMAT_R16G16B16_UINT);
				CASE(UINT32, VK_FORMAT_R32G32B32_UINT);
				CASE(UINT64, VK_FORMAT_R64G64B64_UINT);
			default:
				break;
			}
			break;
		case data_layout::type::VEC4:
		case data_layout::type::MAT2x4:
		case data_layout::type::MAT3x4:
		case data_layout::type::MAT4:
			switch (type.ct)
			{
				CASE(FLOAT16, VK_FORMAT_R16G16B16A16_SFLOAT);
				CASE(FLOAT32, VK_FORMAT_R32G32B32A32_SFLOAT);
				CASE(FLOAT64, VK_FORMAT_R64G64B64A64_SFLOAT);
				CASE(INT8, VK_FORMAT_R8G8B8A8_SINT);
				CASE(INT16, VK_FORMAT_R16G16B16A16_SINT);
				CASE(INT32, VK_FORMAT_R32G32B32A32_SINT);
				CASE(INT64, VK_FORMAT_R64G64B64A64_SINT);
				CASE(UINT8, VK_FORMAT_R8G8B8A8_UINT);
				CASE(UINT16, VK_FORMAT_R16G16B16A16_UINT);
				CASE(UINT32, VK_FORMAT_R32G32B32A32_UINT);
				CASE(UINT64, VK_FORMAT_R64G64B64A64_UINT);
			default:
				break;
			}
			break;
		default:
			break;
		}
		return format;
	}
#undef CASE

	inline VkPipelineVertexInputStateCreateInfo get_vertex_inputs(const data_layout* vertex_layouts, std::uint32_t n_layouts)
	{
		VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		vertex_input_info.vertexBindingDescriptionCount = n_layouts;
		std::uint32_t attribute_description_count = 0;
		VkVertexInputBindingDescription* binding_descriptions = t_calloc<VkVertexInputBindingDescription>(n_layouts);
		for (std::uint32_t i = 0; i < n_layouts; i++)
		{
			VkVertexInputBindingDescription desc = {};
			binding_descriptions[i].binding = 0;
			binding_descriptions[i].stride = static_cast<std::uint32_t>(vertex_layouts[i].size());
			binding_descriptions[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			
			attribute_description_count += vertex_layouts[i].vector_count();
		}
		vertex_input_info.pVertexBindingDescriptions = binding_descriptions;
		vertex_input_info.vertexAttributeDescriptionCount = attribute_description_count;
		VkVertexInputAttributeDescription* attribute_descriptions = 
			t_calloc<VkVertexInputAttributeDescription>(attribute_description_count);

		std::uint32_t c = 0;
		for (std::uint32_t i = 0; i < n_layouts; i++)
		{
			std::uint32_t offset = 0;
			const data_layout_internal& layouts = reinterpret_cast<const data_layout_internal&>(vertex_layouts[i]);
			for (std::uint32_t k = 0; k < vertex_layouts[i].vector_count(); k++)
			{
				std::uint32_t iterations; //matrix inputs are passed as several vectors
				switch (layouts[k].t)
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

				const VkFormat format = to_vk_format(layouts[k]);
				INTERNAL_ASSERT(format != VK_FORMAT_UNDEFINED, "Invalid type.");
				for (std::uint32_t j = 0; j < iterations; j++)
				{
					attribute_descriptions[c].binding = i;
					attribute_descriptions[c].location = k + j;
					attribute_descriptions[c].format = format;
					attribute_descriptions[c].offset = offset;

					offset += layouts[k].size() / iterations;
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
		case spiral::shader::UNIFORM:		 return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case spiral::shader::STORAGE:		 return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case spiral::shader::SAMPLER:		 return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case spiral::shader::TEXTURE:		 return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case spiral::shader::IMAGE:			 return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case spiral::shader::SAMPLER_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		case spiral::shader::IMAGE_BUFFER:	 return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		case spiral::shader::SAMPLER_SHADOW: return VK_DESCRIPTOR_TYPE_SAMPLER; 
		case spiral::shader::NONE:
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
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

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
		VkDescriptorPool* out_descriptor_pool, VkDescriptorSet** out_descriptor_sets)
	{
		std::vector<std::vector<VkDescriptorSetLayoutBinding>> sets;
		std::unordered_map<VkDescriptorType, VkDescriptorPoolSize> pool_sizes_0;
		std::unordered_map<VkDescriptorType, VkDescriptorPoolSize> pool_sizes_1;

		//parse bindings
		for (const shader* p : shaders)
		{
			const internal_shader& cast_shader = reinterpret_cast<const internal_shader&>(*p);
			std::uint32_t n_descriptors = 0;
			shader::descriptor_data* descriptors = cast_shader.get_descriptors(&n_descriptors);

			for (std::uint32_t i = 0; i < n_descriptors; i++)
			{
				for (std::uint32_t s = static_cast<std::uint32_t>(sets.size()); s < descriptors[i].set + 1; s++) sets.emplace_back();
				for (std::uint32_t b = static_cast<std::uint32_t>(sets[descriptors[i].set].size()); b < descriptors[i].binding + 1; b++)
					sets[descriptors[i].set].push_back({});

				VkDescriptorSetLayoutBinding& layout_binding = sets[descriptors[i].set][descriptors[i].binding];
				VkDescriptorType descriptor_type = to_vk_descriptor_type(descriptors[i].type);
				if (layout_binding.descriptorType)
				{
					if (layout_binding.descriptorType == descriptor_type && layout_binding.descriptorCount == descriptors[i].count)
						layout_binding.stageFlags |= cast_shader.get_stage();
					else INTERNAL_ASSERT(false, "Different descriptor for the same binding in different shaders");
				}
				else
				{
					const std::uint32_t count = descriptors[i].count ? descriptors[i].count : 1;

					layout_binding.binding = descriptors[i].binding;
					layout_binding.descriptorCount = count;
					layout_binding.stageFlags = cast_shader.get_stage();
					layout_binding.pImmutableSamplers = nullptr; //TODO deal with this

					if (descriptors[i].set == 0)
					{
						pool_sizes_0[descriptor_type].descriptorCount += count;

						if (descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) 
							layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
						else
							layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
					}
					else
					{
						if (descriptors[i].set == 1) pool_sizes_1[descriptor_type].descriptorCount += count;

						layout_binding.descriptorType = descriptor_type;
					}
				}
			}
		}

		for (auto& set : sets)
			set.erase(std::remove_if(set.begin(), set.end(), 
				[](const VkDescriptorSetLayoutBinding& b) { return static_cast<bool>(b.descriptorType); }), set.end());

		//init descriptor set layouts
		*out_n_descriptor_set_layouts = 0;
		*out_descriptor_set_layouts = t_malloc<VkDescriptorSetLayout>(sets.size());
		std::uint32_t max_sets = 0;
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

				if (*out_n_descriptor_set_layouts < pipeline_descriptor_sets) max_sets++;
			}
			(*out_n_descriptor_set_layouts)++;
		}

		//init descriptor sets
		if (max_sets)
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

	//TODO: change some of the initialisation to smaller functions and put them in the initialiser list
	graphics_pipeline::graphics_pipeline(device& owner, const std::vector<const shader*>& shaders) : owner(&owner)
	{
		LOG_INTERNAL_INFO("Initialising new graphics pipeline..");

		VkPipelineVertexInputStateCreateInfo vertex_input_info = get_vertex_inputs(shaders[0]->get_inputs(), 1);

		init_descriptors(owner.handle, shaders, n_descriptor_pool_sizes, descriptor_pool_sizes,
			&n_descriptor_set_layouts, &descriptor_set_layouts, &descriptor_pool, &descriptor_sets);

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; //primitives
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(owner.main_swapchain.extent.width);
		viewport.height = static_cast<float>(owner.main_swapchain.extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f; //always between 1 and 0

		VkRect2D scissor = {}; //all values measured in pixels
		scissor.offset = { 0, 0 };
		scissor.extent = owner.main_swapchain.extent;

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

		//dynamicstates missing

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 0;
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
		pipeline_info.pDynamicState = nullptr; // Optional
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

		for (std::uint32_t i = 0; i < stage_count; i++)
		{
			vkDestroyShaderModule(owner.handle, shader_stages[i].module, nullptr);
			std::free(const_cast<char*>(shader_stages[i].pName)); //a bit messy, but shouldn't cause issues
		}

		std::free(shader_stages);
		std::free(const_cast<VkVertexInputBindingDescription*>(vertex_input_info.pVertexBindingDescriptions)); //a bit messy, but shouldn't cause issues
		std::free(const_cast<VkVertexInputAttributeDescription*>(vertex_input_info.pVertexAttributeDescriptions)); //a bit messy, but shouldn't cause issues
	}

	graphics_pipeline::~graphics_pipeline()
	{
		if (handle != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(owner->handle, handle, nullptr);
			vkDestroyPipelineLayout(owner->handle, pipeline_layout, nullptr);

			if (descriptor_pool != VK_NULL_HANDLE)
				vkDestroyDescriptorPool(owner->handle, descriptor_pool, nullptr);

			for (std::uint32_t i = 0; i < n_descriptor_set_layouts; i++)
				if (descriptor_set_layouts[i] != VK_NULL_HANDLE)
					vkDestroyDescriptorSetLayout(owner->handle, descriptor_set_layouts[i], nullptr);
			std::free(descriptor_set_layouts);
			std::free(descriptor_pool_sizes[0]);
			std::free(descriptor_pool_sizes[1]);
			std::free(descriptor_sets);
		}
	}

	inline VkIndexType to_vk_index_type(object_resource::index_format format)
	{
		switch (format)
		{
		case object_resource::index_format::UINT8:
			return VK_INDEX_TYPE_UINT8_EXT;
		case object_resource::index_format::UINT16:
			return VK_INDEX_TYPE_UINT16;
		case object_resource::index_format::UINT32:
			return VK_INDEX_TYPE_UINT32;
		default:
			INTERNAL_ASSERT(false, "Index format not implemented");
		}
		return VK_INDEX_TYPE_NONE_KHR;
	}

	void graphics_pipeline::record_commands(VkCommandBuffer& buffer, std::uint8_t current_frame)
	{
		vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, handle);

		const std::uint32_t frame_descriptor_count = object_descriptor_set_capacity + (n_descriptor_pool_sizes[1] ? 1 : 0);

		if (n_descriptor_pool_sizes[1])
			vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
				&descriptor_sets[frame_descriptor_count * current_frame], 0, nullptr);

		for (std::size_t i = 0; i < objects.size(); i++)
		{
			//perobject sets
			//vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptor_sets[i], 0, nullptr);
				
			const object& target = objects[i];
			buffer_binding_args v_args = owner->get_memory().get_binding_args(*target.mesh);
			vkCmdBindVertexBuffers(buffer, 0, 1, &v_args.buffer, &v_args.offset);
			if (target.mesh->index_type() == object_resource::index_format::NONE)
			{
				vkCmdDraw(buffer, target.mesh->count(), target.instances, 0, 0);
			}
			else
			{
				buffer_binding_args i_args = owner->get_memory().get_index_binding_args(*target.mesh);
				const VkIndexType index_t = to_vk_index_type(target.mesh->index_type());
				vkCmdBindIndexBuffer(buffer, i_args.buffer, i_args.offset, index_t);
				vkCmdDrawIndexed(buffer, target.mesh->count(), target.instances, 0, 0, 0);
			}
		}
	}

	void graphics_pipeline::link(const object_resource& mesh)
	{
		object obj = {};
		obj.mesh = &mesh;
		obj.instances = 1;

		objects.push_back(obj);
	}

	void graphics_pipeline::link(const object_resource& mesh, const device_data& instances)
	{
		object obj = {};
		obj.mesh = &mesh;
		obj.instances = 1;

		objects.push_back(obj);
	}
}
