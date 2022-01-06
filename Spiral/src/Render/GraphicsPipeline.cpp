#include "pch.hpp"

#include <Spiral/Render/GraphicsPipeline.hpp>

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
		/*
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);
		*/
		return attributeDescriptions;
	}
};

namespace Spiral
{
	void GraphicsPipeline::init(VkDevice logicalHandle, Swapchain swapchain, VkCommandPool commandPool, const Shader* shaders, uint16_t nShaders)
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
		poolSize.descriptorCount = swapchain.nImages;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = swapchain.nImages;

		if (vkCreateDescriptorPool(logicalHandle, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) //TODO: according to the example this shouldn't be so high, but I haven't found any relevant dependency
		{
			DEBUG_BREAK;
			valid = false;
			vkDestroyDescriptorSetLayout(logicalHandle, descriptorSetLayout, nullptr);
			return;
		}

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = swapchain.imageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;	//multisampling
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;	//clear to black, dont know how it works
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

		if (vkCreateRenderPass(logicalHandle, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			DEBUG_BREAK;
			valid = false;
			vkDestroyDescriptorPool(logicalHandle, descriptorPool, nullptr);
			vkDestroyDescriptorSetLayout(logicalHandle, descriptorSetLayout, nullptr);
			return;
		}

		VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
		std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions = Vertex::getAttributeDescriptions(); //TODO: take array out

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // Optional, but not really
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // Optional, but not really

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; //primitives
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapchain.extent.width;
		viewport.height = (float)swapchain.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f; //always between 1 and 0

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = swapchain.extent;

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
			vkDestroyRenderPass(logicalHandle, renderPass, nullptr);
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
		pipelineInfo.pVertexInputState = &vertexInputInfo;
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

		for (i = 0; i < nShaders; i++)
		{
			vkDestroyShaderModule(logicalHandle, modules[i], nullptr);
		}

		frameBuffers = (VkFramebuffer*)malloc(sizeof(VkFramebuffer) * swapchain.nImages);
		for (i = 0; i < swapchain.nImages; i++)
		{
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &swapchain.imageViews[i];
			framebufferInfo.width = swapchain.extent.width;
			framebufferInfo.height = swapchain.extent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(logicalHandle, &framebufferInfo, nullptr, &frameBuffers[i]) != VK_SUCCESS)
			{
				//TODO: cleanup properly
				DEBUG_BREAK;
				valid = false;
				return;
			}
		}

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = swapchain.nImages;

		commandBuffers = (VkCommandBuffer*)malloc(sizeof(VkCommandBuffer) * swapchain.nImages);
		if (vkAllocateCommandBuffers(logicalHandle, &allocInfo, commandBuffers) != VK_SUCCESS)
		{
			//TODO: cleanup properly
			DEBUG_BREAK;
			valid = false;
			return;
		}
		/*
		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = swapchain.nImages;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = swapchain.nImages;

		if (vkCreateDescriptorPool(logicalHandle, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		{
			DEBUG_BREAK;
			valid = false;
			return;
		}
		*/
		descriptorSets = (VkDescriptorSet*)malloc(sizeof(VkDescriptorSet) * swapchain.nImages);
		VkDescriptorSetLayout* layouts = (VkDescriptorSetLayout*)malloc(sizeof(VkDescriptorSetLayout) * swapchain.nImages);
		for (i = 0; i < swapchain.nImages; i++)
		{
			layouts[i] = descriptorSetLayout;
		}
		VkDescriptorSetAllocateInfo setAllocInfo = {};
		setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		setAllocInfo.descriptorPool = descriptorPool;
		setAllocInfo.descriptorSetCount = swapchain.nImages;
		setAllocInfo.pSetLayouts = layouts;
		if (vkAllocateDescriptorSets(logicalHandle, &setAllocInfo, descriptorSets) != VK_SUCCESS)
		{
			//TODO: cleanup properly
			DEBUG_BREAK;
			valid = false;
			return;
		}
		/*
		for (i = 0; i < swapchain.nImages; i++)
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

		extent = swapchain.extent;

		Memory::init(memory, &commandPool);
		SPRL_CORE_INFO("New pipeline initialised");
	}

	void GraphicsPipeline::terminate(VkDevice logicalHandle, uint32_t nImages)
	{
		uint32_t i;
		for (i = 0; i < nImages; i++)
		{
			vkDestroyFramebuffer(logicalHandle, frameBuffers[i], nullptr);
		}
		free(descriptorSets);

		vkDestroyPipeline(logicalHandle, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(logicalHandle, pipelineLayout, nullptr);
		vkDestroyRenderPass(logicalHandle, renderPass, nullptr);
		vkDestroyDescriptorPool(logicalHandle, descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(logicalHandle, descriptorSetLayout, nullptr);

		Memory::terminate(memory);
	}

	void GraphicsPipeline::loadMesh(Mesh mesh)
	{
		Memory::createBuffer(mesh.vertices, mesh.vSize, 0, memory);
		Memory::createBuffer(mesh.indices, mesh.iSize, 0, memory);
	}

	void GraphicsPipeline::draw(size_t frame)
	{
		uint32_t i;
		Memory::flush(memory);

		//vkResetCommandBuffer(commandBuffers[frame], 0);
		if (vkResetCommandBuffer(commandBuffers[frame], 0) != VK_SUCCESS)
		{
			//do stuff
			DEBUG_BREAK;
		}

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(commandBuffers[frame], &beginInfo) != VK_SUCCESS)
		{
			//do stuff
			DEBUG_BREAK;
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = frameBuffers[frame];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = extent;

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffers[frame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffers[frame], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		for (i = 0; i < memory.pools[0].nRanges; i += 2)
		{
			VkBuffer vertexBuffers[] = { memory.pools[0].buffer };
			VkDeviceSize offsets[] = { memory.pools[0].ranges[i].offset };
			vkCmdBindVertexBuffers(commandBuffers[frame], 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(commandBuffers[frame], memory.pools[0].buffer, memory.pools[0].ranges[i + 1].offset, VK_INDEX_TYPE_UINT16);

			//vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

			vkCmdDrawIndexed(commandBuffers[frame], memory.pools[0].ranges[i + 1].size / sizeof(uint16_t), 1, 0, 0, 0);
		}

		vkCmdEndRenderPass(commandBuffers[frame]);

		if (vkEndCommandBuffer(commandBuffers[frame]) != VK_SUCCESS)
		{
			//do stuff
			DEBUG_BREAK;
		}

		Memory::waitFlush(memory);
	}
}