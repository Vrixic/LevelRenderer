#pragma once
#define GATEWARE_ENABLE_GRAPHICS // Enables all Graphics Libraries

#include "../Gateware.h"
#include <iostream>
#include "vulkan/vulkan_core.h"
#include "LevelData.h"

struct GlobalMeshData
{
	/* Globally shared model information */
	Matrix4D View;
	Matrix4D Projection;

	/* Lighting Information */
	Vector4D DirectionalLightDirection;
	Vector4D DirectionalLightColor;
	Vector4D SunAmbient;
	Vector4D CameraWorldPosition;
};

struct LocalMeshData
{
	std::vector<Matrix4D> WorldMatrices;
	std::vector<Material> Materials;
};

class Level
{
private:
	GW::GRAPHICS::GVulkanSurface* VlkSurface;

	VkDevice* Device;
	VkPipelineLayout* PipelineLayout;

	/* API shader storage buffers */
private:
	/* Structured buffers for shaders */
	std::vector<VkBuffer> ShaderLocalStorageHandles;
	std::vector<VkDeviceMemory> ShaderLocalStorageDatas;

	std::vector<VkBuffer> ShaderGlobalStorageHandles;
	std::vector<VkDeviceMemory> ShaderGlobalStorageDatas;

	/* Used to tell vulkan what type of descriptor we want to set */
	VkDescriptorSetLayout ShaderLocalStorageDescSetLayout = nullptr;
	VkDescriptorSetLayout ShaderGlobalStorageDescSetLayout = nullptr;

	/* Used to reserve descriptor sets */
	VkDescriptorPool ShaderLocalStoragePool = nullptr;
	VkDescriptorPool ShaderGlobalStoragePool = nullptr;

	std::vector<VkDescriptorSet> ShaderLocalStorageDescSets;
	std::vector<VkDescriptorSet> ShaderGlobalStorageDescSets;

	LocalMeshData ShaderLocalData;
	GlobalMeshData ShaderGlobalData;

	/* Rendering Data */
private:
	std::string Name;
	LevelData* WorldData;

	bool IsDataLoaded;

public:
	Level(VkDevice* deviceHandle, GW::GRAPHICS::GVulkanSurface* vlkSurface, VkPipelineLayout* pipelineLayout, const char* name)
	{
		Device = deviceHandle;
		VlkSurface = vlkSurface;
		PipelineLayout = pipelineLayout;

		Name = name;
		WorldData = nullptr;

		IsDataLoaded = false;
	}

	~Level()
	{
		for (uint32 i = 0; i < ShaderLocalStorageHandles.size(); ++i)
		{
			vkDestroyBuffer(*Device, ShaderLocalStorageHandles[i], nullptr);
			vkDestroyBuffer(*Device, ShaderGlobalStorageHandles[i], nullptr);

			vkFreeMemory(*Device, ShaderLocalStorageDatas[i], nullptr);
			vkFreeMemory(*Device, ShaderGlobalStorageDatas[i], nullptr);
		}

		vkDestroyDescriptorPool(*Device, ShaderLocalStoragePool, nullptr);
		vkDestroyDescriptorPool(*Device, ShaderGlobalStoragePool, nullptr);

		vkDestroyDescriptorSetLayout(*Device, ShaderLocalStorageDescSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(*Device, ShaderGlobalStorageDescSetLayout, nullptr);

		if (WorldData != nullptr)
		{
			delete WorldData;
		}
	}

public:
	/* Bind before rendering */
	void Bind()
	{
		if (IsDataLoaded)
		{
			WorldData->Bind();

			unsigned int CurrentBuffer;
			VlkSurface->GetSwapchainCurrentImage(CurrentBuffer);

			VkCommandBuffer CommandBuffer;
			VlkSurface->GetCommandBuffer(CurrentBuffer, (void**)&CommandBuffer);

			vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *PipelineLayout, 0, 1, &ShaderGlobalStorageDescSets[CurrentBuffer], 0, nullptr);
			vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *PipelineLayout, 0, 1, &ShaderLocalStorageDescSets[CurrentBuffer], 0, nullptr);
		}
	}

	/* Loads all data needed to render the level */
	void Load(const char* levelPath)
	{
		if (IsDataLoaded)
		{
			std::cout << "\n[Level]: " << Name << " is already loaded in.. Failed to load!";
			return;
		}

		/* Load the file */
		std::vector<RawMeshData> RawData;
		FileHelper::ReadGameLevelFile(levelPath, RawData);
		WorldData = new LevelData(Device, VlkSurface, RawData);

		WorldData->Load();

		/*  ----  */
		uint32 NumOfActiveFrames = 0;
		VlkSurface->GetSwapchainImageCount(NumOfActiveFrames);

		CreateStorageBuffers(NumOfActiveFrames);

		CreateDescriptorSetLayout(ShaderLocalStorageDescSetLayout);
		CreateDescriptorSetLayout(ShaderGlobalStorageDescSetLayout);

		CreateDescriptorPool(NumOfActiveFrames, ShaderLocalStoragePool);
		CreateDescriptorPool(NumOfActiveFrames, ShaderGlobalStoragePool);

		AllocateDescriptorSets(NumOfActiveFrames, ShaderLocalStorageDescSetLayout, ShaderLocalStorageDescSets, ShaderLocalStoragePool);
		AllocateDescriptorSets(NumOfActiveFrames, ShaderGlobalStorageDescSetLayout, ShaderGlobalStorageDescSets,  ShaderGlobalStoragePool);

		LinkDescriptorSetsToBuffer(NumOfActiveFrames, ShaderLocalStorageDescSets, ShaderLocalStorageHandles);
		LinkDescriptorSetsToBuffer(NumOfActiveFrames, ShaderGlobalStorageDescSets, ShaderGlobalStorageHandles);

		IsDataLoaded = true;
	}

	/* Unloads/Flushes all data needed to render */
	void Unload()
	{
		if (!IsDataLoaded)
		{
			std::cout << "\n[Level]: " << Name << " is not loaded in.. Failed to unload!";
			return;
		}

		WorldData->Unload();
		delete WorldData;

		IsDataLoaded = false;
	}

private:
	void CreateStorageBuffers(uint32 NumOfActiveFrames)
	{
		VkPhysicalDevice PhysicalDevice = nullptr;
		VlkSurface->GetPhysicalDevice((void**)PhysicalDevice);

		ShaderLocalStorageHandles.resize(NumOfActiveFrames);
		ShaderLocalStorageDatas.resize(NumOfActiveFrames);

		ShaderGlobalStorageHandles.resize(NumOfActiveFrames);
		ShaderGlobalStorageDatas.resize(NumOfActiveFrames);

		uint32 LocalMeshDataSizeInBytes = ShaderLocalData.Materials.size() * sizeof(Material) + sizeof(Matrix4D) * ShaderLocalData.WorldMatrices.size();
		uint32 GlobalMeshDataSizeInBytes = sizeof(GlobalMeshData);

		for (uint32 i = 0; i < NumOfActiveFrames; ++i)
		{
			GvkHelper::create_buffer(PhysicalDevice, *Device, LocalMeshDataSizeInBytes,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ShaderLocalStorageHandles[i], &ShaderLocalStorageDatas[i]);

			GvkHelper::write_to_buffer(*Device, ShaderLocalStorageDatas[i], &ShaderLocalData, LocalMeshDataSizeInBytes);

			GvkHelper::create_buffer(PhysicalDevice, *Device, GlobalMeshDataSizeInBytes,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ShaderGlobalStorageHandles[i], &ShaderGlobalStorageDatas[i]);

			GvkHelper::write_to_buffer(*Device, ShaderGlobalStorageDatas[i], &ShaderGlobalData, GlobalMeshDataSizeInBytes);
		}
	}

	void CreateDescriptorSetLayout(VkDescriptorSetLayout& descSetLayout)
	{
		VkDescriptorSetLayoutBinding DescSetLayoutBinding = { };
		DescSetLayoutBinding.binding = 0;
		DescSetLayoutBinding.descriptorCount = 1;
		DescSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
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
		vkCreateDescriptorSetLayout(*Device, &DescSetLayoutCreateInfo, nullptr, &descSetLayout);
	}

	void CreateDescriptorPool(uint32 numOfActiveFrames, VkDescriptorPool& descPool)
	{
		VkDescriptorPoolSize DescPoolSize = { };
		DescPoolSize.descriptorCount = numOfActiveFrames;
		DescPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		VkDescriptorPoolCreateInfo DescPoolCreateInfo = { };
		DescPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		DescPoolCreateInfo.pNext = nullptr;
		DescPoolCreateInfo.flags = 0;
		DescPoolCreateInfo.maxSets = numOfActiveFrames;
		DescPoolCreateInfo.poolSizeCount = 1;
		DescPoolCreateInfo.pPoolSizes = &DescPoolSize;

		vkCreateDescriptorPool(*Device, &DescPoolCreateInfo, nullptr, &descPool);
	}

	void AllocateDescriptorSets(uint32 numOfActiveFrames, VkDescriptorSetLayout& descSetLayout, std::vector<VkDescriptorSet>& descSets, VkDescriptorPool& descPool)
	{
		VkDescriptorSetAllocateInfo DescSetAllocateInfo = { };
		DescSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		DescSetAllocateInfo.pNext = nullptr;
		DescSetAllocateInfo.descriptorPool = descPool;
		DescSetAllocateInfo.descriptorSetCount = 1;
		DescSetAllocateInfo.pSetLayouts = &descSetLayout;

		descSets.resize(numOfActiveFrames);
		for (uint32 i = 0; i < numOfActiveFrames; ++i)
		{
			vkAllocateDescriptorSets(*Device, nullptr, &descSets[i]);
		}
	}

	void LinkDescriptorSetsToBuffer(uint32 numOfActiveFrames, std::vector<VkDescriptorSet>& shaderStorageDescSets, std::vector<VkBuffer>& shaderStorageHandles)
	{
		VkDescriptorBufferInfo* DescBufferInfo = new VkDescriptorBufferInfo[numOfActiveFrames];
		for (uint32 i = 0; i < numOfActiveFrames; ++i)
		{
			DescBufferInfo[i].buffer = shaderStorageHandles[0];
			DescBufferInfo[i].offset = 0;
			DescBufferInfo[i].range = VK_WHOLE_SIZE;
		}

		VkWriteDescriptorSet* DescWriteSets = new VkWriteDescriptorSet[numOfActiveFrames];
		for (uint32 i = 0; i < numOfActiveFrames; ++i)
		{
			DescWriteSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			DescWriteSets[i].pNext = nullptr;
			DescWriteSets[i].dstSet = shaderStorageDescSets[i];
			DescWriteSets[i].dstBinding = 0;
			DescWriteSets[i].dstArrayElement = 0;
			DescWriteSets[i].descriptorCount = 1;
			DescWriteSets[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			DescWriteSets[i].pImageInfo = nullptr;
			DescWriteSets[i].pBufferInfo = DescBufferInfo;
			DescWriteSets[i].pTexelBufferView = nullptr;
		}

		// Link 
		vkUpdateDescriptorSets(*Device, numOfActiveFrames, DescWriteSets, 0, nullptr);

		delete DescBufferInfo;
		delete DescWriteSets;
	}
};
