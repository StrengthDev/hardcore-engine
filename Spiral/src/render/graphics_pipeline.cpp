#include <pch.hpp>

#include <spiral/render/device.hpp>
#include <spiral/render/graphics_pipeline.hpp>
/*
//TODO: delet this
#include "glm/glm.hpp"
struct Vertex
{
	glm::vec3 pos;
	//glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		
		//attributeDescriptions[1].binding = 0;
		//attributeDescriptions[1].location = 1;
		//attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		//attributeDescriptions[1].offset = offsetof(Vertex, color);
		
		return attributeDescriptions;
	}
};*/

namespace Spiral
{
	class data_layout_internal : data_layout
	{
	public:
		data_layout_internal() = delete;

		friend VkFormat to_vk_format(const data_layout::value&);
		friend void get_vertex_inputs(const data_layout&,
			std::uint32_t*, VkVertexInputBindingDescription**, std::uint32_t*, VkVertexInputAttributeDescription**);
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
		case data_layout::type::MAT2://TODO
		case data_layout::type::MAT3://TODO
		case data_layout::type::MAT4://TODO
		default:
			break;
		}
		return format;
	}
#undef CASE

	inline void get_vertex_inputs(const data_layout& vertex_layout, 
		std::uint32_t* out_binding_count, VkVertexInputBindingDescription** out_bindings,
		std::uint32_t* out_attribute_description_count, VkVertexInputAttributeDescription** out_descriptions)
	{
		const std::uint32_t binding_count = 1;
		*out_binding_count = binding_count;
		*out_bindings = t_malloc<VkVertexInputBindingDescription>(binding_count);
		(**out_bindings).binding = 0;
		(**out_bindings).stride = vertex_layout.size();
		(**out_bindings).inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		const std::uint32_t attribute_description_count = vertex_layout.count();
		*out_attribute_description_count = attribute_description_count;
		*out_descriptions = t_malloc<VkVertexInputAttributeDescription>(attribute_description_count);
		std::uint32_t offset = 0;
		const data_layout_internal& layout = reinterpret_cast<const data_layout_internal&>(vertex_layout);
		for (std::uint32_t i = 0; i < attribute_description_count; i++)
		{
			(*out_descriptions)[i].binding = 0;
			(*out_descriptions)[i].location = i;

			const VkFormat format = to_vk_format(layout[i]);
			assertm(format != VK_FORMAT_UNDEFINED, "Invalid type.");
			(*out_descriptions)[i].format = format;

			(*out_descriptions)[i].offset = offset;
			offset += layout[i].size();
		}
	}

	graphics_pipeline::graphics_pipeline(device* owner, VkDevice logicalHandle, swapchain& target, VkRenderPass& renderPass,
		VkCommandPool commandPool, const Shader* shaders, uint16_t nShaders, 
		const data_layout vertex_layout, const data_layout object_layout, const data_layout global_layout)
		: owner(owner), logicalHandle(logicalHandle), target(target), vertex_layout(vertex_layout), object_layout(object_layout), global_layout(global_layout)
	{
		uint32_t i;
		valid = true;

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

		if (vkCreateDescriptorSetLayout(logicalHandle, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		{
			DEBUG_BREAK;
			valid = false;
			return;
		}

		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = target.n_images;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = target.n_images;

		if (vkCreateDescriptorPool(logicalHandle, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) //TODO: according to the example this shouldn't be so high, but I haven't found any relevant dependency
		{
			DEBUG_BREAK;
			valid = false;
			vkDestroyDescriptorSetLayout(logicalHandle, descriptorSetLayout, nullptr);
			return;
		}
		std::uint32_t binding_count, attribute_description_count;
		VkVertexInputBindingDescription* bindings;
		VkVertexInputAttributeDescription* attribute_descriptions;
		get_vertex_inputs(vertex_layout, &binding_count, &bindings, &attribute_description_count, &attribute_descriptions);
		//VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
		//std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions = Vertex::getAttributeDescriptions(); //TODO: take array out

		VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexBindingDescriptionCount = binding_count;
		vertex_input_info.pVertexBindingDescriptions = bindings;
		vertex_input_info.vertexAttributeDescriptionCount = attribute_description_count;
		vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; //primitives
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)target.extent.width;
		viewport.height = (float)target.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f; //always between 1 and 0

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = target.extent;

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
		pipelineLayoutInfo.setLayoutCount = 1; // Optional
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		if (vkCreatePipelineLayout(logicalHandle, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			DEBUG_BREAK;
			valid = false;
			vkDestroyDescriptorPool(logicalHandle, descriptorPool, nullptr);
			vkDestroyDescriptorSetLayout(logicalHandle, descriptorSetLayout, nullptr);
			return;
		}

		//TODO: free this
		VkShaderModule* modules = (VkShaderModule*)malloc(sizeof(VkShaderModule) * nShaders);
		VkPipelineShaderStageCreateInfo* shaderStages = (VkPipelineShaderStageCreateInfo*)calloc(nShaders, sizeof(VkPipelineShaderStageCreateInfo));
		for (i = 0; i < nShaders; i++)
		{
			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = shaders[i].size;
			createInfo.pCode = shaders[i].source;

			if (vkCreateShaderModule(logicalHandle, &createInfo, nullptr, &modules[i]) != VK_SUCCESS)
			{
				//TODO: stuff
				DEBUG_BREAK;
				valid = false;
				return;
			}

			shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[i].stage = shaders[i].type;
			shaderStages[i].module = modules[i];
			shaderStages[i].pName = "main";
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
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional
		pipelineInfo.pNext = nullptr;
		pipelineInfo.flags = 0;
		pipelineInfo.pTessellationState = nullptr;

		if (vkCreateGraphicsPipelines(logicalHandle, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
		{
			DEBUG_BREAK;
			vkDestroyPipelineLayout(logicalHandle, pipelineLayout, nullptr);
			vkDestroyRenderPass(logicalHandle, renderPass, nullptr);
			vkDestroyDescriptorPool(logicalHandle, descriptorPool, nullptr);
			vkDestroyDescriptorSetLayout(logicalHandle, descriptorSetLayout, nullptr);
			valid = false;
			return;
		}

		std::free(bindings);
		std::free(attribute_descriptions);

		for (i = 0; i < nShaders; i++)
		{
			vkDestroyShaderModule(logicalHandle, modules[i], nullptr);
		}
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
		descriptorSets = (VkDescriptorSet*)malloc(sizeof(VkDescriptorSet) * target.n_images);
		VkDescriptorSetLayout* layouts = (VkDescriptorSetLayout*)malloc(sizeof(VkDescriptorSetLayout) * target.n_images);
		for (i = 0; i < target.n_images; i++)
		{
			layouts[i] = descriptorSetLayout;
		}
		VkDescriptorSetAllocateInfo setAllocInfo = {};
		setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		setAllocInfo.descriptorPool = descriptorPool;
		setAllocInfo.descriptorSetCount = target.n_images;
		setAllocInfo.pSetLayouts = layouts;
		if (vkAllocateDescriptorSets(logicalHandle, &setAllocInfo, descriptorSets) != VK_SUCCESS)
		{
			//TODO: cleanup properly
			DEBUG_BREAK;
			valid = false;
			return;
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

		extent = target.extent;
		LOG_INTERNAL_INFO("New pipeline initialised");
	}

	graphics_pipeline::~graphics_pipeline()
	{
		free(descriptorSets);

		vkDestroyPipeline(logicalHandle, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(logicalHandle, pipelineLayout, nullptr);
		vkDestroyDescriptorPool(logicalHandle, descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(logicalHandle, descriptorSetLayout, nullptr);
	}

	void graphics_pipeline::record_commands(VkCommandBuffer& buffer)
	{
		vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		/*
		for (i = 0; i < memory.pools[0].n_slots; i += 2)
		{
			VkBuffer vertexBuffers[] = { memory.pools[0].buffer };
			VkDeviceSize offsets[] = { memory.pools[0].ranges[i].offset };
			vkCmdBindVertexBuffers(buffer, 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(buffer, memory.pools[0].buffer, memory.pools[0].ranges[i + 1].offset, VK_INDEX_TYPE_UINT16);

			//vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

			vkCmdDrawIndexed(buffer, memory.pools[0].ranges[i + 1].size / sizeof(uint16_t), 1, 0, 0, 0); //TODO warning conversion from 'VkDeviceSize' to 'uint32_t', possible loss of data
		}*/

		for (std::uint32_t i = 0; i < n_targets; i++)
		{
			object& target = targets[i];
			vkCmdBindVertexBuffers(buffer, 0, 1, &owner->get_memory().pools[target.vertex_buffer].buffer, &target.v_offset);

			if (target.index_type == index_format::NONE)
			{
				vkCmdDraw(buffer, target.count, target.n_instances, 0, 0);
			}
			else
			{
				switch (target.index_type)
				{
				case index_format::UINT8:
					vkCmdBindIndexBuffer(buffer, VK_NULL_HANDLE, target.i_offset, VK_INDEX_TYPE_UINT8_EXT); //TODO
					break;
				case index_format::UINT16:
					vkCmdBindIndexBuffer(buffer, VK_NULL_HANDLE, target.i_offset, VK_INDEX_TYPE_UINT16); //TODO
					break;
				case index_format::UINT32:
					vkCmdBindIndexBuffer(buffer, VK_NULL_HANDLE, target.i_offset, VK_INDEX_TYPE_UINT32); //TODO
					break;
				default:
					assertm(false, "Index format not implemented.");
					break;
				}
				vkCmdDrawIndexed(buffer, target.count, target.n_instances, 0, 0, 0);
			}
		}
	}
}
