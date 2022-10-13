#pragma once
#include "GatewareDefine.h"
#include <iostream>

#include "LevelData.h"
#include "StorageBuffer.h"

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

	std::vector<Matrix4D> WorldMatrices;
	std::vector<Material> Materials;
};

/*--------------------------------------------------DEBUG-------------------------------------------------------*/

struct SceneDataDebug
{
	/* Globally shared model information */
	Matrix4D View;
	Matrix4D Projection;

	/* Lighting Information */
	Vector4D DirectionalLightDirection;
	Vector4D DirectionalLightColor;
	Vector4D SunAmbient;
	Vector4D CameraWorldPosition;

	Matrix4D WorldMatrices;
	Material Materials;
};

/*--------------------------------------------------DEBUG-------------------------------------------------------*/

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

	VkDescriptorSet ShaderStorageDescSet;

	SceneData ShaderSceneData;

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

		ShaderStorageDescSetLayout = nullptr;
		ShaderStoragePool = nullptr;

		Path = path;
		Name = name;
		WorldData = nullptr;

		IsDataLoaded = false;
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

			//vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *PipelineLayout, 0, 1, &ShaderStorageDescSet, 0, nullptr);

			/*--------------------------------------------------DEBUG-------------------------------------------------------*/
			SceneDataDebug Data;
			Data.View = ShaderSceneData.View;
			Data.Projection = ShaderSceneData.Projection;
			Data.WorldMatrices.SetIdentity();
			GvkHelper::write_to_buffer(*Device, ShaderStorageDatas[0], &Data, sizeof(SceneDataDebug));
			vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *PipelineLayout, 0, 1, &ShaderStorageDescSet, 0, nullptr);
			/*--------------------------------------------------DEBUG-------------------------------------------------------*/
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
				ShaderSceneData.WorldMatrices.push_back(RawData[i].WorldMatrices[j]);
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

				ShaderSceneData.Materials.push_back(Mat);
			}
		}

		/* Load Global Stuff */
		GW::MATH::GMatrix Matrix;
		GW::MATH::GMATRIXF View;
		GW::MATH::GMATRIXF Projection
			;
		Matrix.IdentityF(View);
		GW::MATH::GVECTORF Eye = { -156.0f, 130.0f, 142.0f, 1.0f };
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

		ShaderSceneData.View = reinterpret_cast<Matrix4D&>(View);
		ShaderSceneData.Projection = reinterpret_cast<Matrix4D&>(Projection);

		WorldData->Load();

		/*  ----  */
		uint32 NumOfActiveFrames = 0;
		VlkSurface->GetSwapchainImageCount(NumOfActiveFrames);

		uint32 SceneDataSizeInBytes = ShaderSceneData.Materials.size()
			* sizeof(Material) + sizeof(Matrix4D) * ShaderSceneData.WorldMatrices.size()
			+ sizeof(Vector4D) * 4 + sizeof(Matrix4D) * 2;

		/*--------------------------------------------------DEBUG-------------------------------------------------------*/
		NumOfActiveFrames = 1;

		/* Create the storage buffers */	
		GW::GReturn GResult;
		VkPhysicalDevice PhysicalDevice = nullptr;
		if ((GResult = VlkSurface->GetPhysicalDevice((void**)&PhysicalDevice)) != GW::GReturn::SUCCESS)
		{
			std::cout << "\n[Gateware] Failed to get PhysicalDevice....";
			return;
		}

		ShaderStorageHandles.resize(NumOfActiveFrames);
		ShaderStorageDatas.resize(NumOfActiveFrames);

		for (uint32 i = 0; i < NumOfActiveFrames; ++i)
		{
			VkResult VResult = GvkHelper::create_buffer(PhysicalDevice, *Device, SceneDataSizeInBytes,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ShaderStorageHandles[i], &ShaderStorageDatas[i]);

			GvkHelper::write_to_buffer(*Device, ShaderStorageDatas[i], &ShaderSceneData, SceneDataSizeInBytes);
		}

		VkDescriptorSetLayoutBinding DescSetLayoutBinding[2];
		DescSetLayoutBinding[0].binding = 0;
		DescSetLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		DescSetLayoutBinding[0].descriptorCount = 1;
		DescSetLayoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		DescSetLayoutBinding[0].pImmutableSamplers = nullptr;

		// Tells vulkan how many binding we have and where they are 
		VkDescriptorSetLayoutCreateInfo DescSetLayoutCreateInfo = { };
		DescSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		DescSetLayoutCreateInfo.pNext = nullptr;
		DescSetLayoutCreateInfo.flags = 0;
		DescSetLayoutCreateInfo.bindingCount = 1;
		DescSetLayoutCreateInfo.pBindings = DescSetLayoutBinding;

		// Create the descriptor
		VkResult Result = vkCreateDescriptorSetLayout(*Device, &DescSetLayoutCreateInfo, nullptr, &ShaderStorageDescSetLayout);

		VkDescriptorPoolSize DescPoolSize[2];
		DescPoolSize[0].descriptorCount = 1;
		DescPoolSize[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		VkDescriptorPoolCreateInfo DescPoolCreateInfo = { };
		DescPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		DescPoolCreateInfo.pNext = nullptr;
		DescPoolCreateInfo.flags = 0;
		DescPoolCreateInfo.maxSets = 1;
		DescPoolCreateInfo.poolSizeCount = 1;
		DescPoolCreateInfo.pPoolSizes = DescPoolSize;

		Result = vkCreateDescriptorPool(*Device, &DescPoolCreateInfo, nullptr, &ShaderStoragePool);

		VkDescriptorSetAllocateInfo DescSetAllocateInfo = { };
		DescSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		DescSetAllocateInfo.pNext = nullptr;
		DescSetAllocateInfo.descriptorPool = ShaderStoragePool;
		DescSetAllocateInfo.descriptorSetCount = 1;
		DescSetAllocateInfo.pSetLayouts = &ShaderStorageDescSetLayout;

		Result = vkAllocateDescriptorSets(*Device, &DescSetAllocateInfo, &ShaderStorageDescSet);

		VkDescriptorBufferInfo DescBufferInfo[2] = { };
		DescBufferInfo[0].buffer = ShaderStorageHandles[0];
		DescBufferInfo[0].offset = 0;
		DescBufferInfo[0].range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet DescWriteSets[2] = { };
		DescWriteSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		DescWriteSets[0].pNext = nullptr;
		DescWriteSets[0].dstBinding = 0;
		DescWriteSets[0].dstSet = ShaderStorageDescSet;
		DescWriteSets[0].dstArrayElement = 0;
		DescWriteSets[0].descriptorCount = 1;
		DescWriteSets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		DescWriteSets[0].pImageInfo = nullptr;
		DescWriteSets[0].pBufferInfo = &DescBufferInfo[0];
		DescWriteSets[0].pTexelBufferView = nullptr;

		// Link 
		vkUpdateDescriptorSets(*Device, 1, DescWriteSets, 0, nullptr);

		/*--------------------------------------------------DEBUG-------------------------------------------------------*/
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

	std::vector<VkDescriptorSetLayout*> GetShaderStorageDescSetLayouts()
	{
		std::vector<VkDescriptorSetLayout*> Vec;
		Vec.push_back(&ShaderStorageDescSetLayout);

		return Vec;
	}

	GW::MATH::GMATRIXF GetViewMatrix()
	{
		GW::MATH::GMATRIXF Mat = reinterpret_cast<GW::MATH::GMATRIXF&>(ShaderSceneData.View);
		return Mat;
	}

	void SetViewMatrix(GW::MATH::GMATRIXF& mat)
	{
		ShaderSceneData.View = reinterpret_cast<Matrix4D&>(mat.data);
	}

private:

	void CreateStorageBuffer(uint32 numOfActiveFrames, std::vector<VkBuffer>& bufferHandles, std::vector<VkDeviceMemory>& bufferDatas, const void* data, uint32 bufferSize)
	{
		GW::GReturn Result;
		VkPhysicalDevice PhysicalDevice = nullptr;
		if ((Result = VlkSurface->GetPhysicalDevice((void**)&PhysicalDevice)) != GW::GReturn::SUCCESS)
		{
			std::cout << "\n[Gateware] Failed to get PhysicalDevice....";
			return;
		}

		bufferHandles.resize(numOfActiveFrames);
		bufferDatas.resize(numOfActiveFrames);

		for (uint32 i = 0; i < numOfActiveFrames; ++i)
		{
			VkResult VResult = GvkHelper::create_buffer(PhysicalDevice, *Device, bufferSize,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &bufferHandles[i], &bufferDatas[i]);

			GvkHelper::write_to_buffer(*Device, bufferDatas[i], data, bufferSize);
		}
	}

	void CreateDescriptorSetLayout(uint32 numOfLayouts, VkDescriptorSetLayout* descSetLayout, uint32 bindingNum)
	{
		VkDescriptorSetLayoutBinding* DescSetLayoutBinding = new VkDescriptorSetLayoutBinding[numOfLayouts];

		for (uint32 i = 0; i < numOfLayouts; ++i)
		{
			DescSetLayoutBinding[i].binding = bindingNum;
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
		VkResult Result = vkCreateDescriptorSetLayout(*Device, &DescSetLayoutCreateInfo, nullptr, descSetLayout);

		delete[] DescSetLayoutBinding;
	}

	void CreateDescriptorPool(uint32 numOfActiveFrames, VkDescriptorPool& descPool)
	{
		VkDescriptorPoolSize* DescPoolSize = new VkDescriptorPoolSize[numOfActiveFrames];
		for (uint32 i = 0; i < numOfActiveFrames; ++i)
		{
			DescPoolSize[i].descriptorCount = numOfActiveFrames;
			DescPoolSize[i].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		}

		VkDescriptorPoolCreateInfo DescPoolCreateInfo = { };
		DescPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		DescPoolCreateInfo.pNext = nullptr;
		DescPoolCreateInfo.flags = 0;
		DescPoolCreateInfo.maxSets = numOfActiveFrames;
		DescPoolCreateInfo.poolSizeCount = numOfActiveFrames;
		DescPoolCreateInfo.pPoolSizes = DescPoolSize;

		VkResult Result = vkCreateDescriptorPool(*Device, &DescPoolCreateInfo, nullptr, &descPool);

		delete[] DescPoolSize;
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
			std::cout << std::endl;
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
