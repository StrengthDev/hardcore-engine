#include <pch.hpp>

#include <spiral/render/device.hpp>
#include <spiral/render/graphics_pipeline.hpp>

namespace Spiral
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
	//TODO: change some of the initialisation to smaller functions and put them in the initialiser list
	//TODO: remove layout args, they are unnecessary, everything can be extracted from the shaders
	graphics_pipeline::graphics_pipeline(device& owner, const shader** shaders, uint16_t nShaders, 
		const data_layout& vertex_layout, const data_layout& object_layout, const data_layout& global_layout)
		: owner(&owner)
	{
		LOG_INTERNAL_INFO("Initialising new graphics pipeline..");
		VkResult result;

		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &uboLayoutBinding;

		result = vkCreateDescriptorSetLayout(owner.handle, &layoutInfo, nullptr, &descriptorSetLayout);
		if (result != VK_SUCCESS)
		{
			CRASH("Failed to create descriptor set layout", result);
		}

		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = owner.main_swapchain.n_images;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = owner.main_swapchain.n_images;

		result = vkCreateDescriptorPool(owner.handle, &poolInfo, nullptr, &descriptorPool);
		if (result != VK_SUCCESS)
		{
			CRASH("Failed to create descriptor pool", result);
		}

		VkPipelineVertexInputStateCreateInfo vertex_input_info = get_vertex_inputs(&vertex_layout, 1);

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

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = owner.main_swapchain.extent;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

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

		VkPipelineMultisampleStateCreateInfo multisampling = {}; //requires enabling gup feature
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		//DepthStencil missing

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		/* regular alpha blending
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		*/

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		//dynamicstates missing

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0; // Optional
		pipelineLayoutInfo.pSetLayouts = nullptr;// &descriptorSetLayout; // Optional
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		result = vkCreatePipelineLayout(owner.handle, &pipelineLayoutInfo, nullptr, &pipelineLayout);
		if (result != VK_SUCCESS)
		{
			CRASH("Failed to create pipeline layout", result);
		}

		VkPipelineShaderStageCreateInfo* shaderStages = t_calloc<VkPipelineShaderStageCreateInfo>(nShaders);
		for (std::uint16_t i = 0; i < nShaders; i++)
		{
			shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[i].module = internal_shader::create_shader_module(*shaders[i], owner.handle, &shaderStages[i].stage,
				&shaderStages[i].pName);
		}

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = nShaders;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertex_input_info;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr; // Optional
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = owner.render_pass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional
		pipelineInfo.pNext = nullptr;
		pipelineInfo.flags = 0;
		pipelineInfo.pTessellationState = nullptr;

		result = vkCreateGraphicsPipelines(owner.handle, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
		if (result != VK_SUCCESS)
		{
			CRASH("Failed to create pipeline", result);
		}

		for (std::uint16_t i = 0; i < nShaders; i++)
		{
			vkDestroyShaderModule(owner.handle, shaderStages[i].module, nullptr);
			std::free(const_cast<char*>(shaderStages[i].pName)); //a bit messy, but shouldn't cause issues
		}

		std::free(shaderStages);
		std::free(const_cast<VkVertexInputBindingDescription*>(vertex_input_info.pVertexBindingDescriptions)); //a bit messy, but shouldn't cause issues
		std::free(const_cast<VkVertexInputAttributeDescription*>(vertex_input_info.pVertexAttributeDescriptions)); //a bit messy, but shouldn't cause issues
		/*
		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = target.n_images;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = target.n_images;

		if (vkCreateDescriptorPool(logicalHandle, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		{
			DEBUG_BREAK;
			valid = false;
			return;
		}
		*/
		descriptorSets = t_calloc<VkDescriptorSet>(owner.main_swapchain.n_images);
		VkDescriptorSetLayout* layouts = t_calloc<VkDescriptorSetLayout>(owner.main_swapchain.n_images); //TODO both of these should be frames in flight
		for (std::uint32_t i = 0; i < owner.main_swapchain.n_images; i++)
		{
			layouts[i] = descriptorSetLayout;
		}
		VkDescriptorSetAllocateInfo setAllocInfo = {};
		setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		setAllocInfo.descriptorPool = descriptorPool;
		setAllocInfo.descriptorSetCount = owner.main_swapchain.n_images;
		setAllocInfo.pSetLayouts = layouts;

		result = vkAllocateDescriptorSets(owner.handle, &setAllocInfo, descriptorSets);
		if (result != VK_SUCCESS)
		{
			CRASH("Failed to allocate descriptor sets", result);
		}

		/*
		for (i = 0; i < target.n_images; i++)
		{
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject); //Size of total uniform data

			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = descriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo; //(obviously) there can be more than one uniform buffer
			descriptorWrite.pImageInfo = nullptr; // Optional
			descriptorWrite.pTexelBufferView = nullptr; // Optional

			vkUpdateDescriptorSets(logicalHandle, 1, &descriptorWrite, 0, nullptr);
		}
		*/

		if (object_layout.size()) use_instancing = true;
	}

	graphics_pipeline::~graphics_pipeline()
	{
		if (graphicsPipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(owner->handle, graphicsPipeline, nullptr);
			vkDestroyPipelineLayout(owner->handle, pipelineLayout, nullptr);
			vkDestroyDescriptorPool(owner->handle, descriptorPool, nullptr);
			vkDestroyDescriptorSetLayout(owner->handle, descriptorSetLayout, nullptr);

			free(descriptorSets);
		}
	}

	inline VkIndexType to_vkindextype(object_resource::index_format format)
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
			INTERNAL_ASSERT(false, "Index format not implemented.");
			return VK_INDEX_TYPE_NONE_KHR;
		}
	}

	void graphics_pipeline::record_commands(VkCommandBuffer& buffer)
	{
		vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		if (use_instancing)
		{
			for (std::size_t i = 0; i < objects.size(); i++)
			{
				const object_resource* target = objects[i];
				const instance* instance_buffer = instances[i];
				buffer_binding_args v_args = owner->get_memory().get_binding_args(*target);
				vkCmdBindVertexBuffers(buffer, 0, 1, &v_args.buffer, &v_args.offset);
				if (target->index_type == object_resource::index_format::NONE)
				{
					vkCmdDraw(buffer, target->get_count(), instance_buffer->get_count(), 0, 0);
				}
				else
				{
					buffer_binding_args i_args = owner->get_memory().get_binding_args(*instance_buffer); //TODO this is index, not instance
					const VkIndexType index_t = to_vkindextype(target->index_type);
					vkCmdBindIndexBuffer(buffer, i_args.buffer, i_args.offset, index_t);
					vkCmdDrawIndexed(buffer, target->get_count(), instance_buffer->get_count(), 0, 0, 0);
				}
			}
		}
		else
		{
			for (std::size_t i = 0; i < objects.size(); i++)
			{
				const object_resource* target = objects[i];
				buffer_binding_args v_args = owner->get_memory().get_binding_args(*target);
				vkCmdBindVertexBuffers(buffer, 0, 1, &v_args.buffer, &v_args.offset);
				if (target->index_type == object_resource::index_format::NONE)
				{
					vkCmdDraw(buffer, target->get_count(), 1, 0, 0);
				}
				else
				{
					buffer_binding_args i_args = owner->get_memory().get_binding_args(*target); //TODO this is index, not target
					const VkIndexType index_t = to_vkindextype(target->index_type);
					vkCmdBindIndexBuffer(buffer, i_args.buffer, i_args.offset, index_t);
					vkCmdDrawIndexed(buffer, target->get_count(), 1, 0, 0, 0);
				}
			}
		}
	}

	void graphics_pipeline::link(const object_resource& object)
	{
		INTERNAL_ASSERT(!use_instancing, "Objects must have per instance data"); //TODO move to API pipeline object


		objects.push_back(&object);
	}

	void graphics_pipeline::link(const object_resource& object, const instance& instance)
	{
		INTERNAL_ASSERT(use_instancing, "Objects do not have per instance data"); //TODO move to API pipeline object


		objects.push_back(&object);
		instances.push_back(&instance);
	}
}
