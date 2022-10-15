#pragma once
#include "GatewareDefine.h"
#include <vector>
#include "GenericDefines.h"

class VulkanStorageBuffer
{
private:
	VkDevice* Device;
	GW::GRAPHICS::GVulkanSurface* VlkSurface;



public:
	VulkanStorageBuffer() = delete;
	VulkanStorageBuffer(const VulkanStorageBuffer& Other) = delete;

	VulkanStorageBuffer(VkDevice* device, GW::GRAPHICS::GVulkanSurface* vlkSurface, const void* data, uint32 dataSize, uint32 bindNum)
	{
		Device = device;
		VlkSurface = vlkSurface;		
	}

	~VulkanStorageBuffer()
	{
		
	}

public:
	void Bind(VkPipelineLayout* pipeLineLayout)
	{

		
	}

public:
	VkDescriptorSetLayout* GetDescSetLayout()
	{
		
	}
};
