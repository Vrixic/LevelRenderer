#pragma once
#include "GatewareDefine.h"
#include <iostream>

#include "LevelData.h"
#include "StorageBuffer.h"

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

	StorageBuffer* GlobalDataBuffer;
	StorageBuffer* LocalDataBuffer;

	/* Rendering Data */
private:
	std::string Name;
	std::string Path;
	LevelData* WorldData;

	bool IsDataLoaded;

public:
	Level(VkDevice* deviceHandle, GW::GRAPHICS::GVulkanSurface* vlkSurface, VkPipelineLayout* pipelineLayout, const char* name, const char* path)
	{
		Device = deviceHandle;
		VlkSurface = vlkSurface;
		PipelineLayout = pipelineLayout;

		GlobalDataBuffer = nullptr;
		LocalDataBuffer = nullptr;

		Path = path;
		Name = name;
		WorldData = nullptr;

		IsDataLoaded = false;
	}

	Level(const Level& Other) = delete;

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

		if (GlobalDataBuffer != nullptr)
		{
			delete GlobalDataBuffer;
		}

		if (LocalDataBuffer != nullptr)
		{
			delete LocalDataBuffer;
		}
	}

public:
	/* Bind before rendering */
	void Bind()
	{
		if (IsDataLoaded)
		{
			WorldData->Bind();

			GlobalDataBuffer->Bind(PipelineLayout);
			LocalDataBuffer->Bind(PipelineLayout);

			//unsigned int CurrentBuffer;
			//VlkSurface->GetSwapchainCurrentImage(CurrentBuffer);
			//
			//VkCommandBuffer CommandBuffer;
			//VlkSurface->GetCommandBuffer(CurrentBuffer, (void**)&CommandBuffer);
			//
			//vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *PipelineLayout, 0, 1, &ShaderGlobalStorageDescSets[CurrentBuffer], 0, nullptr);
			//vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *PipelineLayout, 0, 1, &ShaderLocalStorageDescSets[CurrentBuffer], 0, nullptr);
		}
	}

	/* Loads all data needed to render the level */
	void Load()
	{
		if (IsDataLoaded)
		{
			std::cout << "\n[Level]: " << Name << " is already loaded in.. Failed to load!";
			return;
		}

		/* Load the file */
		std::vector<RawMeshData> RawData;
		FileHelper::ReadGameLevelFile(Path.c_str(), RawData);
		WorldData = new LevelData(Device, VlkSurface, RawData);

		/* Load the Materices and Materials */
		for (uint32 i = 0; i < RawData.size(); i++)
		{
			for (uint32 j = 0; j < RawData.size(); ++j)
			{
				ShaderLocalData.WorldMatrices.push_back(RawData[i].WorldMatrices[j]);
			}
			
			for (uint32 j = 0; j < RawData[i].MaterialCount; ++j)
			{
				Material Mat;
				Mat.Diffuse = reinterpret_cast<Vector3D&>(RawData[i].Materials[j].attrib.Kd);
				Mat.Dissolve = RawData[i].Materials[j].attrib.d;
				Mat.SpecularColor = reinterpret_cast<Vector3D&>(RawData[i].Materials[j].attrib.Ks);
				Mat.SpecularExponent = RawData[i].Materials[j].attrib.Ns;
				Mat.Ambient = reinterpret_cast<Vector3D&>(RawData[i].Materials[j].attrib.Ka);
				Mat.Sharpness = RawData[i].Materials[j].attrib.sharpness;
				Mat.TransmissionFilter = reinterpret_cast<Vector3D&>(RawData[i].Materials[j].attrib.Tf);
				Mat.OpticalDensity = RawData[i].Materials[j].attrib.Ni;
				Mat.Emissive = reinterpret_cast<Vector3D&>(RawData[i].Materials[j].attrib.Ke);
				Mat.IlluminationModel = RawData[i].Materials[j].attrib.illum;

				ShaderLocalData.Materials.push_back(Mat);
			}
		}

		/* Load Global Stuff */
		GW::MATH::GMatrix Matrix;
		GW::MATH::GMATRIXF View;
		GW::MATH::GMATRIXF Projection
			;
		Matrix.IdentityF(View);
		GW::MATH::GVECTORF Eye = { 0.75f, 0.25f, -1.5f, 1.0f };
		GW::MATH::GVECTORF Target = { 0.15f, 0.75f, 0.0f, 1.0f };
		GW::MATH::GVECTORF Up = { 0.0f, 1.0f, 0.0f, 0.0f };
		Matrix.LookAtLHF(Eye, Target, Up, View);

		Matrix.IdentityF(Projection);
		float AspectRatio = 0.0f;
		VlkSurface->GetAspectRatio(AspectRatio);
		float FOV =Math::DegreesToRadians(65.0f);
		float NearZ = 0.1f;
		float FarZ = 100.0f;
		Matrix.ProjectionDirectXLHF(FOV, AspectRatio, NearZ, FarZ, Projection);

		ShaderGlobalData.View = reinterpret_cast<Matrix4D&>(View);
		ShaderGlobalData.Projection = reinterpret_cast<Matrix4D&>(Projection);

		WorldData->Load();

		uint32 LocalMeshDataSizeInBytes = ShaderLocalData.Materials.size() * sizeof(Material) + sizeof(Matrix4D) * ShaderLocalData.WorldMatrices.size();
		GlobalDataBuffer = new StorageBuffer(Device, VlkSurface, &ShaderGlobalData, sizeof(ShaderGlobalData), 0);
		LocalDataBuffer = new StorageBuffer(Device, VlkSurface, &ShaderLocalData, sizeof(LocalMeshDataSizeInBytes), 1);

		/*  ----  */
		/*uint32 NumOfActiveFrames = 0;
		VlkSurface->GetSwapchainImageCount(NumOfActiveFrames);

		uint32 LocalMeshDataSizeInBytes = ShaderLocalData.Materials.size() * sizeof(Material) + sizeof(Matrix4D) * ShaderLocalData.WorldMatrices.size();
		uint32 GlobalMeshDataSizeInBytes = sizeof(GlobalMeshData);

		CreateStorageBuffers(NumOfActiveFrames, LocalMeshDataSizeInBytes, GlobalMeshDataSizeInBytes);

		CreateDescriptorSetLayout(ShaderLocalStorageDescSetLayout, 1);
		CreateDescriptorSetLayout(ShaderGlobalStorageDescSetLayout, 0);

		CreateDescriptorPool(NumOfActiveFrames, ShaderLocalStoragePool);
		CreateDescriptorPool(NumOfActiveFrames, ShaderGlobalStoragePool);

		AllocateDescriptorSets(NumOfActiveFrames, ShaderLocalStorageDescSetLayout, ShaderLocalStorageDescSets, ShaderLocalStoragePool);
		AllocateDescriptorSets(NumOfActiveFrames, ShaderGlobalStorageDescSetLayout, ShaderGlobalStorageDescSets,  ShaderGlobalStoragePool);

		LinkDescriptorSetsToBuffer(NumOfActiveFrames, ShaderLocalStorageDescSets, ShaderLocalStorageHandles, 1, LocalMeshDataSizeInBytes);
		LinkDescriptorSetsToBuffer(NumOfActiveFrames, ShaderGlobalStorageDescSets, ShaderGlobalStorageHandles, 0, GlobalMeshDataSizeInBytes);*/

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

	std::vector<VkDescriptorSetLayout> GetShaderStorageDescSetLayouts()
	{		
		std::vector<VkDescriptorSetLayout> Vec;
		//Vec.push_back(&ShaderLocalStorageDescSetLayout);
		//Vec.push_back(&ShaderGlobalStorageDescSetLayout);
		Vec.push_back(*GlobalDataBuffer->GetDescSetLayout());
		Vec.push_back(*LocalDataBuffer->GetDescSetLayout());

		return Vec;
	}

private:
	void CreateStorageBuffers(uint32 numOfActiveFrames, uint32 s1, uint32 s2)
	{
		GW::GReturn Result;
		VkPhysicalDevice PhysicalDevice = nullptr;
		if ((Result = VlkSurface->GetPhysicalDevice((void**)&PhysicalDevice)) != GW::GReturn::SUCCESS)
		{
			std::cout << "\n[Gateware] Failed to get PhysicalDevice....";
			return;
		}

		ShaderLocalStorageHandles.resize(numOfActiveFrames);
		ShaderLocalStorageDatas.resize(numOfActiveFrames);

		ShaderGlobalStorageHandles.resize(numOfActiveFrames);
		ShaderGlobalStorageDatas.resize(numOfActiveFrames);

		for (uint32 i = 0; i < numOfActiveFrames; ++i)
		{
			VkResult VResult = GvkHelper::create_buffer(PhysicalDevice, *Device, s1,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ShaderLocalStorageHandles[i], &ShaderLocalStorageDatas[i]);

			GvkHelper::write_to_buffer(*Device, ShaderLocalStorageDatas[i], &ShaderLocalData, s1);

			VkResult Result = GvkHelper::create_buffer(PhysicalDevice, *Device, s2,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ShaderGlobalStorageHandles[i], &ShaderGlobalStorageDatas[i]);

			Result = GvkHelper::write_to_buffer(*Device, ShaderGlobalStorageDatas[i], &ShaderGlobalData, s2);
		}
	}

	void CreateDescriptorSetLayout(VkDescriptorSetLayout& descSetLayout, uint32 bindingNum)
	{
		VkDescriptorSetLayoutBinding DescSetLayoutBinding = { };
		DescSetLayoutBinding.binding = bindingNum;
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
		VkResult Result = vkCreateDescriptorSetLayout(*Device, &DescSetLayoutCreateInfo, nullptr, &descSetLayout);
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

		VkResult Result = vkCreateDescriptorPool(*Device, &DescPoolCreateInfo, nullptr, &descPool);
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
			VkResult Result = vkAllocateDescriptorSets(*Device, &DescSetAllocateInfo, &descSets[i]);
		}
	}

	void LinkDescriptorSetsToBuffer(uint32 numOfActiveFrames, std::vector<VkDescriptorSet>& shaderStorageDescSets, std::vector<VkBuffer>& shaderStorageHandles, uint32 bindingNum, uint32 sizeOfBuffer)
	{
		VkDescriptorBufferInfo* DescBufferInfo = new VkDescriptorBufferInfo[numOfActiveFrames];
		for (uint32 i = 0; i < numOfActiveFrames; ++i)
		{
			DescBufferInfo[i].buffer = shaderStorageHandles[i];
			DescBufferInfo[i].offset = 0;
			DescBufferInfo[i].range = sizeOfBuffer;
		}

		VkWriteDescriptorSet* DescWriteSets = new VkWriteDescriptorSet[numOfActiveFrames];
		for (uint32 i = 0; i < numOfActiveFrames; ++i)
		{
			DescWriteSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			DescWriteSets[i].pNext = nullptr;
			DescWriteSets[i].dstBinding = bindingNum;
			DescWriteSets[i].dstSet = shaderStorageDescSets[i];
			DescWriteSets[i].dstArrayElement = 0;
			DescWriteSets[i].descriptorCount = 1;
			DescWriteSets[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			DescWriteSets[i].pImageInfo = nullptr;
			DescWriteSets[i].pBufferInfo = &DescBufferInfo[i];
			DescWriteSets[i].pTexelBufferView = nullptr;
		}

		// Link 
		vkUpdateDescriptorSets(*Device, numOfActiveFrames, DescWriteSets, 0, nullptr);

		delete[] DescBufferInfo;
		delete[] DescWriteSets;
	}
};
