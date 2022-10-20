#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>
#include "GenericDefines.h"
#include "Math/Vector2D.h"
#include <iostream>

struct VulkanPipeline
{
	std::vector<VkPipelineShaderStageCreateInfo> StageCreateInfos;

	VkPipelineInputAssemblyStateCreateInfo AssemblyStateCreateInfo;

	std::vector<VkVertexInputBindingDescription> VertexBindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> VertexAttributeDescriptions;
	VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo;

	std::vector<VkViewport> Viewports;
	std::vector<VkRect2D> Scissors;
	VkPipelineViewportStateCreateInfo ViewportStateCreateInfo;

	VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo;
	VkPipelineMultisampleStateCreateInfo MultisampleStateCreateInfo;
	VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo;
	VkPipelineColorBlendAttachmentState ColorBlendAttachmentState;
	VkPipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo;

	std::vector<VkDynamicState> DynamicStates;
	VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo;

	std::vector<VkPushConstantRange> PushConstantRanges;
	VkPipelineLayoutCreateInfo LayoutCreateInfo;

	VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo;

public:
	VulkanPipeline()
	{
		ViewportStateCreateInfo = { };
		AssemblyStateCreateInfo = { };
		VertexInputStateCreateInfo = {  };
		RasterizationStateCreateInfo = { };
		MultisampleStateCreateInfo = { };
		DepthStencilStateCreateInfo = { };
		ColorBlendAttachmentState = { };
		ColorBlendStateCreateInfo = { };
		DynamicStateCreateInfo = { };
		LayoutCreateInfo = { };
		GraphicsPipelineCreateInfo = { };
	}

	void AddNewStageCreateInfo(VkShaderStageFlagBits shaderStage, VkShaderModule* shaderModule)
	{
		VkPipelineShaderStageCreateInfo StageCreateInfo = { };
		StageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		StageCreateInfo.stage = shaderStage;
		StageCreateInfo.module = *shaderModule;
		StageCreateInfo.pName = "main";

		StageCreateInfos.push_back(StageCreateInfo);
	}

	void ClearStageCreateInfos()
	{
		StageCreateInfos.clear();
	}

	void AddNewVertexInputBindingDescription(uint32 bindingNum, uint32 stride, VkVertexInputRate vertexInputRate)
	{
		VkVertexInputBindingDescription VertexBindingDescription = {};
		VertexBindingDescription.binding = bindingNum;
		VertexBindingDescription.stride = stride;
		VertexBindingDescription.inputRate = vertexInputRate;

		VertexBindingDescriptions.push_back(VertexBindingDescription);
	}

	void AddNewVertexInputAttributeDescription(uint32 location, uint32 bindingNum, VkFormat format, uint32 offset)
	{
		VkVertexInputAttributeDescription VertexAttribDescription = { };
		VertexAttribDescription.location = location;
		VertexAttribDescription.binding = bindingNum;
		VertexAttribDescription.format = format;
		VertexAttribDescription.offset = offset;

		VertexAttributeDescriptions.push_back(VertexAttribDescription);
	}

	void AddNewViewport(const Vector2D& pos, const Vector2D& widthHeight, const Vector2D& minMaxDepth)
	{
		VkViewport Viewport = { };
		Viewport.x = pos.X;
		Viewport.y = pos.Y;
		Viewport.width = widthHeight.X;
		Viewport.height = widthHeight.Y;
		Viewport.minDepth = minMaxDepth.X;
		Viewport.maxDepth = minMaxDepth.Y;

		Viewports.push_back(Viewport);
	}

	void AddNewScissor(int32 offsetX, int32 offsetY, int32 extentWidth, int32 extentHeight)
	{
		VkRect2D Scissor = { };
		Scissor.offset.x = offsetX;
		Scissor.offset.y = offsetY;
		Scissor.extent.width = extentWidth;
		Scissor.extent.height = extentHeight;

		Scissors.push_back(Scissor);
	}

	void AddNewDynamicState(VkDynamicState dynamicState)
	{
		DynamicStates.push_back(dynamicState);
	}

	void AddNewPushConstantRange(uint32 offset, VkShaderStageFlags stageFlags, uint32 size)
	{
		VkPushConstantRange PushConstantRange = { };
		PushConstantRange.offset = offset;
		PushConstantRange.stageFlags = stageFlags;
		PushConstantRange.size = size;

		PushConstantRanges.push_back(PushConstantRange);
	}

public:
	void SetInputAssemblyStateCreateInfo(VkPrimitiveTopology topology)
	{
		AssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		AssemblyStateCreateInfo.topology = topology;
		AssemblyStateCreateInfo.primitiveRestartEnable = false;
		AssemblyStateCreateInfo.flags = 0;
	}

	void SetVertexInputStateCreateInfo()
	{
		VertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		VertexInputStateCreateInfo.vertexBindingDescriptionCount = VertexBindingDescriptions.size();
		VertexInputStateCreateInfo.pVertexBindingDescriptions = VertexBindingDescriptions.data();
		VertexInputStateCreateInfo.vertexAttributeDescriptionCount = VertexAttributeDescriptions.size();
		VertexInputStateCreateInfo.pVertexAttributeDescriptions = VertexAttributeDescriptions.data();
		VertexInputStateCreateInfo.flags = 0;
	}

	void SetViewportStateCreateInfo()
	{
		ViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		ViewportStateCreateInfo.viewportCount = Viewports.size();
		ViewportStateCreateInfo.pViewports = Viewports.data();
		ViewportStateCreateInfo.scissorCount = Scissors.size();
		ViewportStateCreateInfo.pScissors = Scissors.data();
	}

	void SetDefaultRasterizationStateCreateInfo()
	{
		RasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		RasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		RasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		RasterizationStateCreateInfo.lineWidth = 1.0f;
		RasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		RasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		RasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
		RasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
		RasterizationStateCreateInfo.depthBiasClamp = 0.0f;
		RasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
		RasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
		RasterizationStateCreateInfo.flags = 0;
	}

	void SetRasterizationStateCreateInfo(VkPipelineRasterizationStateCreateInfo& rasterizationStateCreateInfo)
	{
		RasterizationStateCreateInfo = rasterizationStateCreateInfo;
	}

	void SetDefaultMultisampleStateCreateInfo()
	{
		MultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		MultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
		MultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		MultisampleStateCreateInfo.minSampleShading = 1.0f;
		MultisampleStateCreateInfo.pSampleMask = VK_NULL_HANDLE;
		MultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
		MultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
		MultisampleStateCreateInfo.flags = 0;
	}

	void SetMultisampleStateCreateInfo(VkPipelineMultisampleStateCreateInfo& multisampleCreatInfo)
	{
		MultisampleStateCreateInfo = multisampleCreatInfo;
	}

	void SetDefaultDepthStencilStateCreateInfo()
	{
		DepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		DepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
		DepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
		DepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		DepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
		DepthStencilStateCreateInfo.minDepthBounds = 0.0f;
		DepthStencilStateCreateInfo.maxDepthBounds = 1.0f;
		DepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
		DepthStencilStateCreateInfo.flags = 0;
	}

	void SetDepthStencilStateCreateInfo(VkPipelineDepthStencilStateCreateInfo& depthStencilStateCreateInfo)
	{
		DepthStencilStateCreateInfo = depthStencilStateCreateInfo;
	}

	void SetDefaultColorBlendAttachmentState()
	{
		ColorBlendAttachmentState.colorWriteMask = 0xF;
		ColorBlendAttachmentState.blendEnable = VK_FALSE;
		ColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		ColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
		ColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		ColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		ColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
		ColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	void SetColorBlendAttachmentState(VkPipelineColorBlendAttachmentState& colorBlendAttachmentState)
	{
		ColorBlendAttachmentState = colorBlendAttachmentState;
	}

	void SetDefaultColorBlendStateCreateInfo()
	{
		ColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		ColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
		ColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		ColorBlendStateCreateInfo.attachmentCount = 1;
		ColorBlendStateCreateInfo.pAttachments = &ColorBlendAttachmentState;
		ColorBlendStateCreateInfo.blendConstants[0] = 0.0f;
		ColorBlendStateCreateInfo.blendConstants[1] = 0.0f;
		ColorBlendStateCreateInfo.blendConstants[2] = 0.0f;
		ColorBlendStateCreateInfo.blendConstants[3] = 0.0f;
		ColorBlendStateCreateInfo.flags = 0;
	}

	void SetColorBlendStateCreateInfo(VkPipelineColorBlendStateCreateInfo& colorBlendStateCreateInfo)
	{
		ColorBlendStateCreateInfo = colorBlendStateCreateInfo;
	}

	void SetDynamicStateCreateInfo()
	{
		DynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		DynamicStateCreateInfo.dynamicStateCount = DynamicStates.size();
		DynamicStateCreateInfo.pDynamicStates = DynamicStates.data();
		DynamicStateCreateInfo.flags = 0;
	}

	void SetLayoutCreateInfo(uint32 setLayoutCount, VkDescriptorSetLayout* descSetLayouts)
	{
		LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		LayoutCreateInfo.pNext = nullptr;
		LayoutCreateInfo.setLayoutCount = setLayoutCount;
		LayoutCreateInfo.pSetLayouts = descSetLayouts;
		LayoutCreateInfo.flags = 0;

		LayoutCreateInfo.pushConstantRangeCount = PushConstantRanges.size();
		LayoutCreateInfo.pPushConstantRanges = PushConstantRanges.data();
	}

	void SetGraphicsPipelineCreateInfo(VkPipelineLayout* inPipelineLayout, VkRenderPass* inRenderPass)
	{
		GraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		GraphicsPipelineCreateInfo.stageCount = StageCreateInfos.size();
		GraphicsPipelineCreateInfo.pStages = StageCreateInfos.data();
		GraphicsPipelineCreateInfo.pInputAssemblyState = &AssemblyStateCreateInfo;
		GraphicsPipelineCreateInfo.pVertexInputState = &VertexInputStateCreateInfo;
		GraphicsPipelineCreateInfo.pViewportState = &ViewportStateCreateInfo;
		GraphicsPipelineCreateInfo.pRasterizationState = &RasterizationStateCreateInfo;
		GraphicsPipelineCreateInfo.pMultisampleState = &MultisampleStateCreateInfo;
		GraphicsPipelineCreateInfo.pDepthStencilState = &DepthStencilStateCreateInfo;
		GraphicsPipelineCreateInfo.pColorBlendState = &ColorBlendStateCreateInfo;
		GraphicsPipelineCreateInfo.pDynamicState = &DynamicStateCreateInfo;
		GraphicsPipelineCreateInfo.layout = *inPipelineLayout;
		GraphicsPipelineCreateInfo.renderPass = *inRenderPass;
		GraphicsPipelineCreateInfo.subpass = 0;
		GraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		GraphicsPipelineCreateInfo.flags = 0;
	}

public:
	void CreatePipelineLayout(VkDevice* inDevice, VkPipelineLayout& outPipelineLayout)
	{
		VkResult Result = vkCreatePipelineLayout(*inDevice, &LayoutCreateInfo,
			nullptr, &outPipelineLayout);

		if (Result != VK_SUCCESS)
		{
			std::cout << "\n[VulkanPipeline]: failed to create pipeline layout...";
		}
	}

	void CreateGraphicsPipelines(VkDevice* inDevice, VkPipeline& outPipeline)
	{
		VkResult Result = vkCreateGraphicsPipelines(*inDevice, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &outPipeline);

		if (Result != VK_SUCCESS)
		{
			std::cout << "\n[VulkanPipeline]: failed to create pipeline layout...";
		}
	}

};
