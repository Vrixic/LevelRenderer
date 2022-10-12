#pragma once
#include "GatewareDefine.h"
#include <vector>
#include "GenericDefines.h"

class StorageBuffer
{
private:
	VkDevice* Device;
	GW::GRAPHICS::GVulkanSurface* VlkSurface;

	std::vector<VkBuffer> BufferHandles;
	std::vector<VkDeviceMemory> BufferDatas;

	VkDescriptorSetLayout BufferDescSetLayout = nullptr;

	VkDescriptorPool BufferDescPool = nullptr;

	std::vector<VkDescriptorSet> BufferDescSets;

	const void* BufferMemory = nullptr;

public:
	StorageBuffer() = delete;
	StorageBuffer(const StorageBuffer& Other) = delete;

	StorageBuffer(VkDevice* device, GW::GRAPHICS::GVulkanSurface* vlkSurface, const void* data, uint32 dataSize, uint32 bindNum)
	{
		Device = device;
		VlkSurface = vlkSurface;

		BufferMemory = data;

		VkResult VResult;

		uint32 NumOfActiveFrames = 0;
		VlkSurface->GetSwapchainImageCount(NumOfActiveFrames);

		GW::GReturn Result;
		VkPhysicalDevice PhysicalDevice = nullptr;
		if ((Result = VlkSurface->GetPhysicalDevice((void**)&PhysicalDevice)) != GW::GReturn::SUCCESS)
		{
			std::cout << "\n[StorageBuffer] Failed to get PhysicalDevice..";
			return;
			return;
		}

		BufferHandles.resize(NumOfActiveFrames);
		BufferDatas.resize(NumOfActiveFrames);

		for (uint32 i = 0; i < NumOfActiveFrames; ++i)
		{
			VResult = GvkHelper::create_buffer(PhysicalDevice, *Device, dataSize,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &BufferHandles[i], &BufferDatas[i]);

			if (VResult != VK_SUCCESS)
			{
				std::cout << "\n[StorageBuffer]: Failed to create a buffer... index " << i;
				return;
			}

			VResult = GvkHelper::write_to_buffer(*Device, BufferDatas[i], BufferMemory, dataSize);

			if (VResult != VK_SUCCESS)
			{
				std::cout << "\n[StorageBuffer]: Failed to write to a buffer... index " << i;
				return;
			}
		}

		VkDescriptorSetLayoutBinding DescSetLayoutBinding = { };
		DescSetLayoutBinding.binding = bindNum;
		DescSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		DescSetLayoutBinding.descriptorCount = 1;
		DescSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		DescSetLayoutBinding.pImmutableSamplers = nullptr;

		// Tells vulkan how many binding we have and where they are 
		VkDescriptorSetLayoutCreateInfo DescSetLayoutCreateInfo = { };
		DescSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		DescSetLayoutCreateInfo.pNext = nullptr;
		DescSetLayoutCreateInfo.flags = 0;
		DescSetLayoutCreateInfo.bindingCount = 1;
		DescSetLayoutCreateInfo.pBindings = &DescSetLayoutBinding;

		// Create the descriptor
		VResult = vkCreateDescriptorSetLayout(*Device, &DescSetLayoutCreateInfo, nullptr, &BufferDescSetLayout);
		if (VResult != VK_SUCCESS)
		{
			std::cout << "\n[StorageBuffer]: Failed to create a descriptor set layout..";
			return;
		}

		VkDescriptorPoolSize DescPoolSize = { };
		DescPoolSize.descriptorCount = NumOfActiveFrames;
		DescPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		VkDescriptorPoolCreateInfo DescPoolCreateInfo = { };
		DescPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		DescPoolCreateInfo.pNext = nullptr;
		DescPoolCreateInfo.flags = 0;
		DescPoolCreateInfo.maxSets = NumOfActiveFrames;
		DescPoolCreateInfo.poolSizeCount = 1;
		DescPoolCreateInfo.pPoolSizes = &DescPoolSize;

		VResult = vkCreateDescriptorPool(*Device, &DescPoolCreateInfo, nullptr, &BufferDescPool);
		if (VResult != VK_SUCCESS)
		{
			std::cout << "\n[StorageBuffer]: Failed to create a descriptor pool..";
			return;
		}

		VkDescriptorSetAllocateInfo DescSetAllocateInfo = { };
		DescSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		DescSetAllocateInfo.pNext = nullptr;
		DescSetAllocateInfo.descriptorPool = BufferDescPool;
		DescSetAllocateInfo.descriptorSetCount = 1;
		DescSetAllocateInfo.pSetLayouts = &BufferDescSetLayout;

		BufferDescSets.resize(NumOfActiveFrames);
		for (uint32 i = 0; i < NumOfActiveFrames; ++i)
		{
			VResult = vkAllocateDescriptorSets(*Device, &DescSetAllocateInfo, &BufferDescSets[i]);
			if (VResult != VK_SUCCESS)
			{
				std::cout << "\n[StorageBuffer]: Failed to allocate a descriptor set.. index " << i;
				return;
			}
		}

		VkDescriptorBufferInfo* DescBufferInfo = new VkDescriptorBufferInfo[NumOfActiveFrames];
		for (uint32 i = 0; i < NumOfActiveFrames; ++i)
		{
			DescBufferInfo[i].buffer = BufferHandles[i];
			DescBufferInfo[i].offset = 0;
			DescBufferInfo[i].range = dataSize;
		}

		VkWriteDescriptorSet* DescWriteSets = new VkWriteDescriptorSet[NumOfActiveFrames];
		for (uint32 i = 0; i < NumOfActiveFrames; ++i)
		{
			DescWriteSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			DescWriteSets[i].pNext = nullptr;
			DescWriteSets[i].dstBinding = bindNum;
			DescWriteSets[i].dstSet = BufferDescSets[i];
			DescWriteSets[i].dstArrayElement = 0;
			DescWriteSets[i].descriptorCount = 1;
			DescWriteSets[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			DescWriteSets[i].pImageInfo = nullptr;
			DescWriteSets[i].pBufferInfo = &DescBufferInfo[i];
			DescWriteSets[i].pTexelBufferView = nullptr;
		}

		// Link 
		vkUpdateDescriptorSets(*Device, NumOfActiveFrames, DescWriteSets, 0, nullptr);

		delete[] DescBufferInfo;
		delete[] DescWriteSets;
	}

	~StorageBuffer()
	{
		for (uint32 i = 0; i < BufferHandles.size(); ++i)
		{
			vkDestroyBuffer(*Device, BufferHandles[i], nullptr);
			vkFreeMemory(*Device, BufferDatas[i], nullptr);
		}

		vkDestroyDescriptorPool(*Device, BufferDescPool, nullptr);
		vkDestroyDescriptorSetLayout(*Device, BufferDescSetLayout, nullptr);
	}

public:
	void Bind(VkPipelineLayout* pipeLineLayout)
	{

		unsigned int CurrentBuffer;
		VlkSurface->GetSwapchainCurrentImage(CurrentBuffer);

		VkCommandBuffer CommandBuffer;
		VlkSurface->GetCommandBuffer(CurrentBuffer, (void**)&CommandBuffer);

		vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeLineLayout, 0, 1, &BufferDescSets[CurrentBuffer], 0, nullptr);
	}

public:
	VkDescriptorSetLayout* GetDescSetLayout()
	{
		return &BufferDescSetLayout;
	}
};
