#pragma once
#include "GatewareDefine.h"
#include <iostream>

#include "LevelData.h"
#include "StorageBuffer.h"

#define DEBUG 1

#if DEBUG
#define VK_ASSERT(r)											\
	if (r != VK_SUCCESS)										\
	{															\
		std::cout << "[VK_ASSERT] Failed in file: " << __FILE__;			\
		std::cout << " at line: " << __LINE__ << std::endl;		\
	}															
#endif // DEBUG

#define MAX_SUBMESH_PER_DRAW 512

struct SceneData
{
	/* Globally shared model information */
	Matrix4D View;
	Matrix4D Projection;

	/* Lighting Information */
	Vector4D DirectionalLightDirection;
	Vector4D DirectionalLightColor;
	Vector4D SunAmbient;
	Vector4D CameraWorldPosition;

	Matrix4D WorldMatrices[MAX_SUBMESH_PER_DRAW];
	Material Materials[MAX_SUBMESH_PER_DRAW];
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
	std::vector<VkBuffer> ShaderStorageHandles;
	std::vector<VkDeviceMemory> ShaderStorageDatas;

	/* Used to tell vulkan what type of descriptor we want to set */
	VkDescriptorSetLayout ShaderStorageDescSetLayout;

	/* Used to reserve descriptor sets */
	VkDescriptorPool ShaderStoragePool;

	std::vector<VkDescriptorSet> ShaderStorageDescSets;

	/* Rendering Data */
private:
	/* Level Data */
	std::string Name;
	std::string Path;

	LevelData* WorldData;

	bool IsDataLoaded;

	/* Scene Data */
	SceneData* ShaderSceneData;

	uint64 SceneDataSizeInBytes;

public:
	Level(VkDevice* deviceHandle, GW::GRAPHICS::GVulkanSurface* vlkSurface, VkPipelineLayout* pipelineLayout, const char* name, const char* path)
	{
		Device = deviceHandle;
		VlkSurface = vlkSurface;
		PipelineLayout = pipelineLayout;

		ShaderStorageDescSetLayout = nullptr;
		ShaderStoragePool = nullptr;

		Path = path;
		Name = name;
		WorldData = nullptr;

		IsDataLoaded = false;

		SceneDataSizeInBytes = 0;

		ShaderSceneData = new SceneData;
	}

	Level(const Level& Other) = delete;

	~Level()
	{
		for (uint32 i = 0; i < ShaderStorageHandles.size(); ++i)
		{
			vkDestroyBuffer(*Device, ShaderStorageHandles[i], nullptr);
			vkFreeMemory(*Device, ShaderStorageDatas[i], nullptr);
		}

		vkDestroyDescriptorSetLayout(*Device, ShaderStorageDescSetLayout, nullptr);
		vkDestroyDescriptorPool(*Device, ShaderStoragePool, nullptr);

		if (WorldData != nullptr)
		{
			delete WorldData;
		}

		delete ShaderSceneData;
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

			GvkHelper::write_to_buffer(*Device, ShaderStorageDatas[CurrentBuffer], ShaderSceneData, SceneDataSizeInBytes);
			vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *PipelineLayout, 0, 1, &ShaderStorageDescSets[CurrentBuffer], 0, nullptr);
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
				ShaderSceneData->WorldMatrices[j] = RawData[i].WorldMatrices[j];
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

				ShaderSceneData->Materials[j] = Mat;
			}
		}

		/* Load Global Stuff */
		GW::MATH::GMatrix Matrix;
		GW::MATH::GMATRIXF View;
		GW::MATH::GMATRIXF Projection
			;
		Matrix.IdentityF(View);
		GW::MATH::GVECTORF Eye = { -472.0f, 250.0f, 457.0f, 1.0f };
		GW::MATH::GVECTORF Target = { 0.0f, 0.0f, 0.0f, 1.0f };
		GW::MATH::GVECTORF Up = { 0.0f, 1.0f, 0.0f, 0.0f };
		Matrix.LookAtLHF(Eye, Target, Up, View);

		Matrix.IdentityF(Projection);
		float AspectRatio = 0.0f;
		VlkSurface->GetAspectRatio(AspectRatio);
		float FOV = Math::DegreesToRadians(65.0f);
		float NearZ = 0.1f;
		float FarZ = 1000.0f;
		Matrix.ProjectionDirectXLHF(FOV, AspectRatio, NearZ, FarZ, Projection);

		ShaderSceneData->View = reinterpret_cast<Matrix4D&>(View);
		ShaderSceneData->Projection = reinterpret_cast<Matrix4D&>(Projection);

		WorldData->Load();

		/*  ----  */
		uint32 NumOfActiveFrames = 0;
		VlkSurface->GetSwapchainImageCount(NumOfActiveFrames);

		SceneDataSizeInBytes = sizeof(SceneData);

		/* Resize storage vectors */
		ShaderStorageDescSets.resize(NumOfActiveFrames);
		ShaderStorageHandles.resize(NumOfActiveFrames);
		ShaderStorageDatas.resize(NumOfActiveFrames);

		/* Create the storage buffers */
		CreateStorageBuffers(NumOfActiveFrames, ShaderStorageHandles, ShaderStorageDatas, ShaderSceneData, SceneDataSizeInBytes);

		/* Create only 1 descriptor set layout for our storage buffers */
		CreateDescriptorSetLayouts(1, &ShaderStorageDescSetLayout);

		/* Create only one pool for our descriptor sets */
		VkDescriptorPoolSize DescPoolSize[1];
		DescPoolSize[0].descriptorCount = 1;
		DescPoolSize[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		VkDescriptorPoolCreateInfo DescPoolCreateInfo = { };
		DescPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		DescPoolCreateInfo.pNext = nullptr;
		DescPoolCreateInfo.flags = 0;
		DescPoolCreateInfo.maxSets = NumOfActiveFrames;
		DescPoolCreateInfo.poolSizeCount = 1;
		DescPoolCreateInfo.pPoolSizes = DescPoolSize;

		VK_ASSERT(vkCreateDescriptorPool(*Device, &DescPoolCreateInfo, nullptr, &ShaderStoragePool));

		for (uint32 i = 0; i < NumOfActiveFrames; ++i)
		{
			AllocateDescriptorSet(1, &ShaderStorageDescSetLayout, &ShaderStorageDescSets[i], &ShaderStoragePool);
			LinkDescriptorSetToBuffer(0, SceneDataSizeInBytes, &ShaderStorageDescSets[i], &ShaderStorageHandles[i]);
		}

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

	/* Returns the desc set layouts for the main pipeline */
	std::vector<VkDescriptorSetLayout*> GetShaderStorageDescSetLayouts()
	{
		std::vector<VkDescriptorSetLayout*> Vec;
		Vec.push_back(&ShaderStorageDescSetLayout);

		return Vec;
	}

	/*--------------------------------------------------DEBUG-------------------------------------------------------*/

	GW::MATH::GMATRIXF GetViewMatrix()
	{
		GW::MATH::GMATRIXF Mat = reinterpret_cast<GW::MATH::GMATRIXF&>(ShaderSceneData->View);
		return Mat;
	}

	void SetViewMatrix(GW::MATH::GMATRIXF& mat)
	{
		ShaderSceneData->View = reinterpret_cast<Matrix4D&>(mat.data);
	}

	/*--------------------------------------------------DEBUG-------------------------------------------------------*/

private:

	void CreateStorageBuffers(const uint32 numBuffers, std::vector<VkBuffer>& bufferHandles, std::vector<VkDeviceMemory>& bufferDatas, const void* data, const uint64& bufferSize)
	{
		GW::GReturn Result;
		VkPhysicalDevice PhysicalDevice = nullptr;
		if ((Result = VlkSurface->GetPhysicalDevice((void**)&PhysicalDevice)) != GW::GReturn::SUCCESS)
		{
			std::cout << "\n[Gateware] Failed to get PhysicalDevice....";
			return;
		}

		/* Resize vectors incase their not already resized */
		if (bufferHandles.size() < numBuffers)
		{
			bufferHandles.resize(numBuffers);
		}
		if (bufferDatas.size() < numBuffers)
		{
			bufferDatas.resize(numBuffers);
		}

		VkResult VResult;
		for (uint32 i = 0; i < numBuffers; ++i)
		{
			VK_ASSERT(GvkHelper::create_buffer(PhysicalDevice, *Device, bufferSize,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &bufferHandles[i], &bufferDatas[i]));

			VK_ASSERT(GvkHelper::write_to_buffer(*Device, bufferDatas[i], data, bufferSize));
		}
	}

	void CreateDescriptorSetLayouts(uint32 numOfLayouts, VkDescriptorSetLayout* descSetLayout)
	{
		VkDescriptorSetLayoutBinding* DescSetLayoutBinding = new VkDescriptorSetLayoutBinding[numOfLayouts];

		for (uint32 i = 0; i < numOfLayouts; ++i)
		{
			DescSetLayoutBinding[i].binding = i;
			DescSetLayoutBinding[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			DescSetLayoutBinding[i].descriptorCount = 1;
			DescSetLayoutBinding[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			DescSetLayoutBinding[i].pImmutableSamplers = nullptr;
		}

		// Tells vulkan how many binding we have and where they are 
		VkDescriptorSetLayoutCreateInfo DescSetLayoutCreateInfo = { };
		DescSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		DescSetLayoutCreateInfo.pNext = nullptr;
		DescSetLayoutCreateInfo.flags = 0;
		DescSetLayoutCreateInfo.bindingCount = numOfLayouts;
		DescSetLayoutCreateInfo.pBindings = DescSetLayoutBinding;

		// Create the descriptor
		VK_ASSERT(vkCreateDescriptorSetLayout(*Device, &DescSetLayoutCreateInfo, nullptr, descSetLayout));

		delete[] DescSetLayoutBinding;
	}

	void AllocateDescriptorSet(uint32 numDescriptors, VkDescriptorSetLayout* descSetLayout, VkDescriptorSet* descSet, VkDescriptorPool* descPool)
	{
		VkDescriptorSetAllocateInfo DescSetAllocateInfo = { };
		DescSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		DescSetAllocateInfo.pNext = nullptr;
		DescSetAllocateInfo.descriptorPool = *descPool;
		DescSetAllocateInfo.descriptorSetCount = 1;
		DescSetAllocateInfo.pSetLayouts = descSetLayout;

		VK_ASSERT(vkAllocateDescriptorSets(*Device, &DescSetAllocateInfo, descSet));
	}

	void LinkDescriptorSetToBuffer(uint32 bindingNum, uint64 sizeOfBuffer, VkDescriptorSet* descSet, VkBuffer* bufferHandle)
	{
		VkDescriptorBufferInfo DescBufferInfo = { };
		DescBufferInfo.buffer = *bufferHandle;
		DescBufferInfo.offset = 0;
		DescBufferInfo.range = sizeOfBuffer;

		VkWriteDescriptorSet DescWriteSets = { };
		DescWriteSets.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		DescWriteSets.pNext = nullptr;
		DescWriteSets.dstBinding = 0;
		DescWriteSets.dstSet = *descSet;
		DescWriteSets.dstArrayElement = 0;
		DescWriteSets.descriptorCount = 1;
		DescWriteSets.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		DescWriteSets.pImageInfo = nullptr;
		DescWriteSets.pBufferInfo = &DescBufferInfo;
		DescWriteSets.pTexelBufferView = nullptr;

		// Link 
		vkUpdateDescriptorSets(*Device, 1, &DescWriteSets, 0, nullptr);
	}
};
