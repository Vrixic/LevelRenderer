#pragma once
#include "GatewareDefine.h"
#include <iostream>

#include "LevelData.h"
//#include "StorageBuffer.h"
#define KHRONOS_STATIC
#include <ktxvulkan.h>
#include <ktx.h>
#include "StaticMesh.h"

#define DEBUG 1

#if DEBUG
#define VK_ERROR(r)											\
	if (r != VK_SUCCESS)										\
	{															\
		std::cout << "[VK_ERROR] Failed in file: " << __FILE__;\
		std::cout << " at line: " << __LINE__ << std::endl;		\
	}	

#define GW_ERROR(g)											\
	if (g != GW::GReturn::SUCCESS)								\
	{															\
		std::cout << "[GW_ERROR] Failed in file: " << __FILE__;\
		std::cout << " at line: " << __LINE__ << std::endl;		\
	}	

#define KTX_ERROR(k)											\
	if (k != KTX_error_code::KTX_SUCCESS)								\
	{															\
		std::cout << "[KTX_ERROR] Failed in file: " << __FILE__;\
		std::cout << " at line: " << __LINE__ << std::endl;		\
	}	
#endif // DEBUG

#define MAX_SUBMESH_PER_DRAW 512
#define MAX_LIGHTS_PER_DRAW 16
#define MAX_TEXTURES_PER_DRAW 20

struct Texture
{
	VkSampler Sampler;
	ktxVulkanTexture Texture;
	VkImageView View;
	//VkDescriptorSet DescSet;
};

struct SceneData
{
	/* Globally shared model information */
<<<<<<< HEAD
	Matrix4D View[3];
=======
	Matrix4D View[2];
>>>>>>> a376bcaca055c08eaa3440b4c38d28bc8fba93cc
	Matrix4D Projection;

	/* Lighting Information */
	Vector4D DirectionalLightDirection;
	Vector4D DirectionalLightColor;
	Vector4D SunAmbient;
	Vector4D CameraWorldPosition;

	Matrix4D WorldMatrices[MAX_SUBMESH_PER_DRAW];
	Material Materials[MAX_SUBMESH_PER_DRAW];

	PointLight PointLights[MAX_LIGHTS_PER_DRAW];

	SpotLight SpotLights[MAX_LIGHTS_PER_DRAW];

	/* 16-byte padding for the lights,
	* X component -> num of point lights
	* Y component -> num of spot lights
	*/
	Vector4D NumOfLights;
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
	VkDescriptorSetLayout ShaderTextureDescSetLayout;

	/* Used to reserve descriptor sets */
	VkDescriptorPool ShaderStoragePool;

	std::vector<VkDescriptorSet> ShaderStorageDescSets;
	std::vector<VkDescriptorSet> ShaderTextureDescSets;

	/* Rendering Data */
private:
	/* Level Data */
	std::string Name;
	std::string Path;

	LevelData* WorldData;

	bool IsDataLoaded;

	/* Scene Data */
public:
	SceneData* ShaderSceneData;

private:
	uint64 SceneDataSizeInBytes;

	std::vector<Texture> Textures;

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
	}

	Level(const Level& Other) = delete;

	~Level()
	{
		Unload();
	}

public:
	/* Binds everything before rendering */
	void Bind()
	{
		if (IsDataLoaded)
		{
			WorldData->Bind();

			unsigned int CurrentBuffer;
			VlkSurface->GetSwapchainCurrentImage(CurrentBuffer);

			VkCommandBuffer CommandBuffer;
			VlkSurface->GetCommandBuffer(CurrentBuffer, (void**)&CommandBuffer);

			//ShaderSceneData->CameraWorldPosition = ShaderSceneData->View[0];

			/* Bind Shader data desc set*/
			GvkHelper::write_to_buffer(*Device, ShaderStorageDatas[CurrentBuffer], ShaderSceneData, SceneDataSizeInBytes);
			vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *PipelineLayout, 0, 1, &ShaderStorageDescSets[CurrentBuffer], 0, nullptr);

			if (Textures.size() > 1)
			{
				vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *PipelineLayout, 1, 1, &ShaderTextureDescSets[CurrentBuffer], 0, nullptr);
			}
		}
	}

	/* Loads all data needed to render the level */
	std::vector<StaticMesh> Load()
	{
		if (IsDataLoaded)
		{
			std::cout << "\n[Level]: " << Name << " is already loaded in.. Failed to load!";
			return { };
		}

		/* Load the file */
		std::vector<RawMeshData> RawData;
		FileHelper::ReadGameLevelFile(Path.c_str(), RawData);

		return Load(RawData);
	}
<<<<<<< HEAD

	std::vector<StaticMesh> Load(std::vector<RawMeshData>& rawData)
	{
		if (IsDataLoaded)
		{
			std::cout << "\n[Level]: " << Name << " is already loaded in.. Failed to load!";
			return { };
		}

		/* Load the file */
		std::vector<StaticMesh> StaticMeshes;

		SetupGlobalSceneDataVars(rawData, StaticMeshes);
		WorldData = new LevelData(Device, VlkSurface, rawData);

=======

	std::vector<StaticMesh> Load(std::vector<RawMeshData>& rawData)
	{
		if (IsDataLoaded)
		{
			std::cout << "\n[Level]: " << Name << " is already loaded in.. Failed to load!";
			return { };
		}

		/* Load the file */
		std::vector<StaticMesh> StaticMeshes;
		SetupGlobalSceneDataVars(rawData, StaticMeshes);

		WorldData = new LevelData(Device, VlkSurface, rawData);
>>>>>>> a376bcaca055c08eaa3440b4c38d28bc8fba93cc
		WorldData->Load();

		/*  ----  */
		uint32 NumOfActiveFrames = 0;
		VlkSurface->GetSwapchainImageCount(NumOfActiveFrames);

		SceneDataSizeInBytes = sizeof(SceneData);

		/* Resize storage vectors */
		ShaderStorageDescSets.resize(NumOfActiveFrames);
		ShaderTextureDescSets.resize(NumOfActiveFrames);
		ShaderStorageHandles.resize(NumOfActiveFrames);
		ShaderStorageDatas.resize(NumOfActiveFrames);

		/* Create the storage buffers */
		CreateStorageBuffers(NumOfActiveFrames, ShaderStorageHandles, ShaderStorageDatas, ShaderSceneData, SceneDataSizeInBytes);

		/* Create only one pool for our descriptor sets */
		VkDescriptorPoolSize DescPoolSize[2];
		DescPoolSize[0].descriptorCount = 1;
		DescPoolSize[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		DescPoolSize[1].descriptorCount = Textures.size();
		DescPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		VkDescriptorPoolCreateInfo DescPoolCreateInfo = { };
		DescPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		DescPoolCreateInfo.pNext = nullptr;
		DescPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
		DescPoolCreateInfo.maxSets = NumOfActiveFrames + (Textures.size() * NumOfActiveFrames); // 5 for texture sets
		DescPoolCreateInfo.poolSizeCount = 2;
		DescPoolCreateInfo.pPoolSizes = DescPoolSize;

		VK_ERROR(vkCreateDescriptorPool(*Device, &DescPoolCreateInfo, nullptr, &ShaderStoragePool));

		// Creatae bindless global descriptor layout
		{
			VkDescriptorSetLayoutBinding LayoutBinding[2];
			/* Storage buffer binding */
			LayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			LayoutBinding[0].descriptorCount = 1;
			LayoutBinding[0].binding = 0;
			LayoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			LayoutBinding[0].pImmutableSamplers = nullptr;

			/* Texture buffer binding */
			LayoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			LayoutBinding[1].descriptorCount = Textures.size();
			LayoutBinding[1].binding = 0;
			LayoutBinding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			LayoutBinding[1].pImmutableSamplers = nullptr;

			VkDescriptorSetLayoutCreateInfo LayoutCreateInfo[2];
			/* Storage buffer binding */
			LayoutCreateInfo[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			LayoutCreateInfo[0].bindingCount = 1;
			LayoutCreateInfo[0].pBindings = &LayoutBinding[0];
			LayoutCreateInfo[0].flags = 0;// VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
			LayoutCreateInfo[0].pNext = nullptr;

			/* Texture buffer binding */
			LayoutCreateInfo[1].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			LayoutCreateInfo[1].bindingCount = 1;
			LayoutCreateInfo[1].pBindings = &LayoutBinding[1];
			LayoutCreateInfo[1].flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;

			VkDescriptorSetLayoutBindingFlagsCreateInfoEXT ExtendedInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT, nullptr };
			VkDescriptorBindingFlags BindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
				VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;

			ExtendedInfo.bindingCount = 1;
			ExtendedInfo.pBindingFlags = &BindlessFlags;

			LayoutCreateInfo[1].pNext = &ExtendedInfo;

			VK_ERROR(vkCreateDescriptorSetLayout(*Device, &LayoutCreateInfo[0], nullptr, &ShaderStorageDescSetLayout));
			VK_ERROR(vkCreateDescriptorSetLayout(*Device, &LayoutCreateInfo[1], nullptr, &ShaderTextureDescSetLayout));
		}

		for (uint32 i = 0; i < NumOfActiveFrames; ++i)
		{
			AllocateDescriptorSet(&ShaderStorageDescSetLayout, &ShaderStorageDescSets[i], &ShaderStoragePool);
			LinkDescriptorSetToBuffer(0, SceneDataSizeInBytes, &ShaderStorageDescSets[i], &ShaderStorageHandles[i]);

		}

		uint32 NumWritesPerFrame = Textures.size();
		VkWriteDescriptorSet* BindLessDescriptorWrites = new VkWriteDescriptorSet[NumWritesPerFrame];
		VkDescriptorImageInfo* BindLessImageInfo = new VkDescriptorImageInfo[NumWritesPerFrame];

		for (uint32 i = 0; i < NumOfActiveFrames; ++i)
		{
			VkDescriptorSetAllocateInfo AllocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
			AllocateInfo.descriptorPool = ShaderStoragePool;
			AllocateInfo.descriptorSetCount = 1;
			AllocateInfo.pSetLayouts = &ShaderTextureDescSetLayout;

			VkDescriptorSetVariableDescriptorCountAllocateInfoEXT CountInfo
			{
				VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT
			};
			uint32 max_binding = Textures.size();
			CountInfo.descriptorSetCount = 1;
			// This number is the max allocatable count
			CountInfo.pDescriptorCounts = &max_binding;
			AllocateInfo.pNext = &CountInfo;

			VkResult VResult = vkAllocateDescriptorSets(*Device, &AllocateInfo, &ShaderTextureDescSets[i]);
			VK_ERROR(VResult);

			for (uint32 j = 0; j < NumWritesPerFrame; ++j)
			{
				BindLessDescriptorWrites[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				BindLessDescriptorWrites[j].descriptorCount = 1;
				BindLessDescriptorWrites[j].dstArrayElement = j;
				BindLessDescriptorWrites[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				BindLessDescriptorWrites[j].dstSet = ShaderTextureDescSets[i];
				BindLessDescriptorWrites[j].dstBinding = 0;
				BindLessDescriptorWrites[j].pNext = nullptr;

				BindLessImageInfo[j].sampler = Textures[j].Sampler;
				BindLessImageInfo[j].imageView = Textures[j].View;
				BindLessImageInfo[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				BindLessDescriptorWrites[j].pImageInfo = &BindLessImageInfo[j];
			}

			vkUpdateDescriptorSets(*Device, NumWritesPerFrame, BindLessDescriptorWrites, 0, nullptr);
		}

		delete[] BindLessDescriptorWrites;
		delete[] BindLessImageInfo;

		IsDataLoaded = true;

		std::cout << "[Level]: " << Name << " was loaded...";

		return StaticMeshes;
	}

	/* Unloads all data from gpu and cpu for the current level */
	void Unload()
	{
		DestroyGPUData();

		if (WorldData != nullptr)
		{
			delete WorldData;
			WorldData = nullptr;
		}

		delete ShaderSceneData;
		ShaderSceneData = nullptr;

		IsDataLoaded = false;
	}

	/* Returns the desc set layouts for the main pipeline */
	std::vector<VkDescriptorSetLayout*> GetShaderStorageDescSetLayouts()
	{
		std::vector<VkDescriptorSetLayout*> Vec;
		Vec.push_back(&ShaderStorageDescSetLayout);

		/*--------------------------------------------------TESTING-------------------------------------------------------*/
		Vec.push_back(&ShaderTextureDescSetLayout);
		/*--------------------------------------------------TESTING-------------------------------------------------------*/

		return Vec;
	}

	/*--------------------------------------------------DEBUG-------------------------------------------------------*/

	void SetNewLevel(const char* levelPath)
	{
		if (IsDataLoaded)
		{
			std::cout << "\n[Level]: cannot load another level before unloading previous...\n";
			return;
		}

		Path = levelPath;
	}

	/*
	* Tip: Always use optimal tiled images for rendering
	*/
	void LoadTexture(const char* filePath, Texture& outTexture)
	{
		VkQueue GraphicsQueue = nullptr;
		VkCommandPool CommandPool = nullptr;
		VkPhysicalDevice PhysicalDevice = nullptr;

		GW_ERROR(VlkSurface->GetGraphicsQueue((void**)&GraphicsQueue));
		GW_ERROR(VlkSurface->GetCommandPool((void**)&CommandPool));
		GW_ERROR(VlkSurface->GetPhysicalDevice((void**)&PhysicalDevice));

		// Temp vars for KTX
		ktxTexture* KTexture = nullptr;
		ktxVulkanDeviceInfo VulkanDeviceInfo;

		// Used to transfer texture from CPU memory to GPU
		KTX_ERROR(ktxVulkanDeviceInfo_Construct(&VulkanDeviceInfo, PhysicalDevice, *Device,
			GraphicsQueue, CommandPool, nullptr));

		// Load texture into CPU memory from file
		KTX_ERROR(ktxTexture_CreateFromNamedFile(filePath,
			KTX_TEXTURE_CREATE_NO_FLAGS, &KTexture));

		// This gets mad if you don't encode/save the .ktx file in a format Vulkan likes
		KTX_ERROR(ktxTexture_VkUploadEx(KTexture, &VulkanDeviceInfo, &outTexture.Texture,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));


		// after loading all textures you don't need these anymore
		ktxTexture_Destroy(KTexture);
		ktxVulkanDeviceInfo_Destruct(&VulkanDeviceInfo);

		std::cout << "[Texture]: " << filePath << " loaded successfully...\n";
	}

	GW::MATH::GMATRIXF GetViewMatrix1()
	{
		GW::MATH::GMATRIXF Mat = reinterpret_cast<GW::MATH::GMATRIXF&>(ShaderSceneData->View[0]);
		return Mat;
	}

	GW::MATH::GMATRIXF GetViewMatrix2()
	{
		GW::MATH::GMATRIXF Mat = reinterpret_cast<GW::MATH::GMATRIXF&>(ShaderSceneData->View[1]);
		return Mat;
	}

<<<<<<< HEAD
	GW::MATH::GMATRIXF GetViewMatrix3()
	{
		GW::MATH::GMATRIXF Mat = reinterpret_cast<GW::MATH::GMATRIXF&>(ShaderSceneData->View[2]);
		return Mat;
	}

=======
>>>>>>> a376bcaca055c08eaa3440b4c38d28bc8fba93cc
	void SetViewMatrix1(GW::MATH::GMATRIXF& mat)
	{
		ShaderSceneData->View[0] = reinterpret_cast<Matrix4D&>(mat.data);
	}

	void SetViewMatrix2(GW::MATH::GMATRIXF& mat)
	{
		ShaderSceneData->View[1] = reinterpret_cast<Matrix4D&>(mat.data);
<<<<<<< HEAD
	}

	void SetViewMatrix3(GW::MATH::GMATRIXF& mat)
	{
		ShaderSceneData->View[2] = reinterpret_cast<Matrix4D&>(mat.data);
=======
>>>>>>> a376bcaca055c08eaa3440b4c38d28bc8fba93cc
	}

	void UpdateCameraWorldPosition(GW::MATH::GVECTORF& mat)
	{
		ShaderSceneData->CameraWorldPosition = reinterpret_cast<Vector4D&>(mat);
	}

	void SetProjectionMatrix(GW::MATH::GMATRIXF& mat)
	{
		ShaderSceneData->Projection = reinterpret_cast<Matrix4D&>(mat.data);
	}

	LevelData* GetLevelData()
	{
		return WorldData;
	}

	/*--------------------------------------------------DEBUG-------------------------------------------------------*/

private:

	void DestroyGPUData()
	{
		for (uint32 i = 0; i < ShaderStorageHandles.size(); ++i)
		{
			vkDestroyBuffer(*Device, ShaderStorageHandles[i], nullptr);
			vkFreeMemory(*Device, ShaderStorageDatas[i], nullptr);
		}

		vkDestroyDescriptorSetLayout(*Device, ShaderStorageDescSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(*Device, ShaderTextureDescSetLayout, nullptr);
		vkDestroyDescriptorPool(*Device, ShaderStoragePool, nullptr);

		// Destory the texture
		for (uint32 i = 0; i < Textures.size(); ++i)
		{
			vkDestroySampler(*Device, Textures[i].Sampler, nullptr);
			vkDestroyImageView(*Device, Textures[i].View, nullptr);
			vkDestroyImage(*Device, Textures[i].Texture.image, nullptr);
			vkFreeMemory(*Device, Textures[i].Texture.deviceMemory, nullptr);
		}
	}

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
			VK_ERROR(GvkHelper::create_buffer(PhysicalDevice, *Device, bufferSize,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &bufferHandles[i], &bufferDatas[i]));

			VK_ERROR(GvkHelper::write_to_buffer(*Device, bufferDatas[i], data, bufferSize));
		}
	}

	void CreateDescriptorSetLayouts(uint32 numOfLayouts, uint32 descCount, VkDescriptorSetLayout* descSetLayout, VkDescriptorType descType, VkShaderStageFlags shaderStageFlags)
	{
		VkDescriptorSetLayoutBinding* DescSetLayoutBinding = new VkDescriptorSetLayoutBinding[numOfLayouts];

		for (uint32 i = 0; i < numOfLayouts; ++i)
		{
			DescSetLayoutBinding[i].binding = i;
			DescSetLayoutBinding[i].descriptorType = descType;
			DescSetLayoutBinding[i].descriptorCount = descCount;
			DescSetLayoutBinding[i].stageFlags = shaderStageFlags;
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
		VK_ERROR(vkCreateDescriptorSetLayout(*Device, &DescSetLayoutCreateInfo, nullptr, descSetLayout));

		delete[] DescSetLayoutBinding;
	}

	void AllocateDescriptorSet(VkDescriptorSetLayout* descSetLayout, VkDescriptorSet* descSet, VkDescriptorPool* descPool)
	{
		VkDescriptorSetAllocateInfo DescSetAllocateInfo = { };
		DescSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		DescSetAllocateInfo.pNext = nullptr;
		DescSetAllocateInfo.descriptorPool = *descPool;
		DescSetAllocateInfo.descriptorSetCount = 1;
		DescSetAllocateInfo.pSetLayouts = descSetLayout;

		VkResult Result = vkAllocateDescriptorSets(*Device, &DescSetAllocateInfo, descSet);
		VK_ERROR(Result);
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

	void LinkDescriptorSetToImageBuffer(const uint32 bindingNum, const uint32 dstArrayElement, VkDescriptorSet* descSet, const Texture* texture)
	{
		VkWriteDescriptorSet WriteDescSet = { };
		WriteDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescSet.descriptorCount = 1;
		WriteDescSet.dstArrayElement = dstArrayElement;
		WriteDescSet.dstBinding = bindingNum;
		WriteDescSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		WriteDescSet.dstSet = *descSet;
		VkDescriptorImageInfo DescriptorImageInfo = { texture->Sampler, texture->View, texture->Texture.imageLayout };
		WriteDescSet.pImageInfo = &DescriptorImageInfo;

		vkUpdateDescriptorSets(*Device, 1, &WriteDescSet, 0, nullptr);
	}

	void CreateDefaultSampler(const uint32 maxLod, VkSampler& outSampler)
	{
		VkSamplerCreateInfo SamplerCreateInfo = {};
		SamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		SamplerCreateInfo.flags = 0;
		SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER; // repeat if common
		SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER; // repeat if common
		SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER; // repeat if common
		SamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		SamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		SamplerCreateInfo.mipLodBias = 0;
		SamplerCreateInfo.minLod = 0;
		SamplerCreateInfo.maxLod = maxLod;
		SamplerCreateInfo.anisotropyEnable = VK_FALSE;
		SamplerCreateInfo.maxAnisotropy = 1.0;
		SamplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		SamplerCreateInfo.compareEnable = VK_FALSE;
		SamplerCreateInfo.compareOp = VK_COMPARE_OP_LESS;
		SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		SamplerCreateInfo.pNext = nullptr;

		VK_ERROR(vkCreateSampler(*Device, &SamplerCreateInfo, nullptr, &outSampler));
	}

	void CreateDefaultImageViewFromTexture(Texture* inTexture, VkImageView& outView)
	{
		VkImageViewCreateInfo ImageViewCreateInfo = {};
		// set the non-default values.
		ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageViewCreateInfo.flags = 0;
		ImageViewCreateInfo.components =
		{
			VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A
		};
		ImageViewCreateInfo.image = inTexture->Texture.image;
		ImageViewCreateInfo.format = inTexture->Texture.imageFormat;
		ImageViewCreateInfo.viewType = inTexture->Texture.viewType;
		ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageViewCreateInfo.subresourceRange.layerCount = inTexture->Texture.layerCount;
		ImageViewCreateInfo.subresourceRange.levelCount = inTexture->Texture.levelCount;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.pNext = nullptr;

		VK_ERROR(vkCreateImageView(*Device, &ImageViewCreateInfo, nullptr, &outView));
	}

	void SetupGlobalSceneDataVars(std::vector<RawMeshData>& rawData, std::vector<StaticMesh>& outStaticMeshes)
	{
		ShaderSceneData = new SceneData;

		/* Load Global Stuff */
		GW::MATH::GMatrix Matrix;
		Matrix.Create();

		GW::MATH::GMATRIXF View;
		GW::MATH::GMATRIXF Projection;
		Matrix.IdentityF(View);

		bool HasCamera = false;
		for (uint32 i = 0; i < rawData.size(); ++i)
		{
			if (rawData[i].IsCamera)
			{
				HasCamera = true;
				Matrix.InverseF(reinterpret_cast<GW::MATH::GMATRIXF&>(rawData[i].WorldMatrices[0]), View);
			}
		}

		if (!HasCamera)
		{
			GW::MATH::GVECTORF Eye = { 0.0f, 10.0f, 0.0f, 1.0f };
			GW::MATH::GVECTORF Target = { 0.0f, -25.0f, 0.0f, 1.0f };
			GW::MATH::GVECTORF Up = { 0.0f, 1.0f, 0.0f, 0.0f };
			Matrix.LookAtLHF(Eye, Target, Up, View);
		}

		Matrix.IdentityF(Projection);
		float AspectRatio = 0.0f;
		VlkSurface->GetAspectRatio(AspectRatio);
		float FOV = Math::DegreesToRadians(65.0f);
		float NearZ = 0.1f;
		float FarZ = 1000.0f;
		Matrix.ProjectionDirectXLHF(FOV, AspectRatio, NearZ, FarZ, Projection);

		ShaderSceneData->View[0] = reinterpret_cast<Matrix4D&>(View);
		ShaderSceneData->View[1] = reinterpret_cast<Matrix4D&>(View);
<<<<<<< HEAD
		ShaderSceneData->View[2] = reinterpret_cast<Matrix4D&>(View);

=======
>>>>>>> a376bcaca055c08eaa3440b4c38d28bc8fba93cc
		ShaderSceneData->Projection = reinterpret_cast<Matrix4D&>(Projection);

		Vector3D DirLight(0.0f, -0.6899f, -0.7239f);
		ShaderSceneData->DirectionalLightDirection = Vector4D(DirLight, 1.0f);
		ShaderSceneData->SunAmbient = Vector4D(0.25f, 0.25f, 0.35f, 1.0f);
		ShaderSceneData->DirectionalLightColor = Vector4D(0.9f, 0.9f, 1.0f, 1.0f);

		Mesh TempMesh;
		uint32 MaterialOffset = 0;
		uint32 WorldMatricesOffset = 0;
		uint32 StaticMeshIndex = 0;
		uint32 TextureOffset = 0;

		ShaderSceneData->NumOfLights = 0;
		PointLight PLight;
		SpotLight SLight;

		/* store the texture paths so we can load them in later */
		std::vector<std::string> TexturePaths;
		TexturePaths.push_back("../Assets/Textures/DefaultDiffuseMap.ktx");

		srand(time(NULL));

		Vector4D SpotLightColors[] =
		{
			{1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}
		};
		Vector4D PointLightColors[] =
		{
			{0, 1, 0.898f, 1}, {1, 0, 0.719f, 1},
			{1, 0.380f, 0, 1}, {0, 1, 0.078, 1}
		};

		/* Load the Materices and Materials */
		for (uint32 i = 0; i < rawData.size(); i++)
		{
			bool HasTextures = false;

			if (rawData[i].IsCamera)
			{
				continue;
			}

			if (rawData[i].IsLight)
			{
				switch (rawData[i].Light)
				{
				case Directional:
				{
					/* Get direction vector from matrix */
					Vector4D DirectionVector = (rawData[i].WorldMatrices[0])[2];
					//DirectionVector.W = 0.2f;

					ShaderSceneData->DirectionalLightDirection = DirectionVector;
					ShaderSceneData->SunAmbient = (rawData[i].WorldMatrices[0])[1];
					break;
				}

				case Point:
				{
					PLight.Position = rawData[i].WorldMatrices[0][3];
					PLight.Color = (rawData[i].WorldMatrices[0])[1];//PointLightColors[static_cast<uint32>(ShaderSceneData->NumOfLights.X)];

					/*PLight.Color = Vector4D((rand() / static_cast<float>(RAND_MAX)),
						(rand() / static_cast<float>(RAND_MAX)),
						(rand() / static_cast<float>(RAND_MAX)), 1.0f);*/

					PLight.AddStrength(2.0f);
					PLight.SetRadius(6.0f);

					ShaderSceneData->PointLights[static_cast<uint32>(ShaderSceneData->NumOfLights.X)] = PLight;
					ShaderSceneData->NumOfLights.X += 1.0f;

					break;
				}

				case Spot:
				{
					/* Get direction vector from matrix */
					Vector4D Position = rawData[i].WorldMatrices[0][3];
					Vector4D DirectionVector = rawData[i].WorldMatrices[0][2];
					DirectionVector.Normalize();

					SLight.Position = Position;
					SLight.ConeDirection = DirectionVector;

					SLight.Color = (rawData[i].WorldMatrices[0])[1];//SpotLightColors[static_cast<uint32>(ShaderSceneData->NumOfLights.Y)];


					SLight.AddStrength(2.5f);
					SLight.SetInnerConeRatio((rawData[i].WorldMatrices[0])(1, 3));
					SLight.SetOuterConeRatio((rawData[i].WorldMatrices[0])(2, 3));

					ShaderSceneData->SpotLights[static_cast<uint32>(ShaderSceneData->NumOfLights.Y)] = SLight;
					ShaderSceneData->NumOfLights.Y += 1.0f;
					break;
				}
				}

				continue;
			}

			outStaticMeshes.push_back(StaticMesh(rawData[i].MaterialCount > 0, HasTextures,
				rawData[i].VertexCount, rawData[i].IndexCount, rawData[i].MaterialCount, MaterialOffset,
				rawData[i].MeshCount, rawData[i].InstanceCount, WorldMatricesOffset,
				&ShaderSceneData->WorldMatrices[WorldMatricesOffset]));
<<<<<<< HEAD

			/* Make the debug box for the mesh */
			//outStaticMeshes[StaticMeshIndex].DebugVertexStart = WorldData->DebugBoxVertexStart + (StaticMeshIndex * 8);
			//outStaticMeshes[StaticMeshIndex].DebugVertexStart = WorldData->DebugBoxIndexStart + (StaticMeshIndex * 24);
=======
>>>>>>> a376bcaca055c08eaa3440b4c38d28bc8fba93cc

			uint32 TexOffsetPerMesh = 0; // This offset keeps in track of how many meshes actually had a texture
			for (uint32 j = 0; j < rawData[i].MeshCount; ++j)
			{
				uint32 TexMapOffset = 0;

				TempMesh.IndexCount = rawData[i].Meshes[j].drawInfo.indexCount;
				TempMesh.IndexOffset = rawData[i].Meshes[j].drawInfo.indexOffset;
				TempMesh.Name = rawData[i].Meshes[j].name;
				TempMesh.MaterialIndex = rawData[i].Meshes[j].materialIndex;

				/* Diffuse TextureID */
				if (rawData[i].Materials[rawData[i].Meshes[j].materialIndex].DiffuseMap.size() > 0)
				{
					TempMesh.DiffuseTextureIndex = (j + TextureOffset) + 1; // account for default diffuse texture
					TexturePaths.push_back(rawData[i].Materials[rawData[i].Meshes[j].materialIndex].DiffuseMap);
					TexOffsetPerMesh++;
					TexMapOffset++;

					HasTextures = true;
				}
				else
				{
					TempMesh.DiffuseTextureIndex = -1;
				}

				/* Specular TextureID */
				//if (rawData[i].Materials[rawData[i].Meshes[j].materialIndex].SpecularMap.size() > 0)
				//{
				//	TempMesh.SpecularTextureIndex = (j + TextureOffset + TexMapOffset) + 1; // account for default diffuse texture
				//	TexturePaths.push_back(rawData[i].Materials[rawData[i].Meshes[j].materialIndex].SpecularMap);
				//	TexOffsetPerMesh++;
				//	TexMapOffset++;
				//
				//	HasTextures = true;
				//}
				//else
				//{
				//	TempMesh.SpecularTextureIndex = -1;
				//}

				//outStaticMeshes[StaticMeshIndex].Meshes.push_back(TempMesh);
				outStaticMeshes[StaticMeshIndex].AddSubMesh(TempMesh);
			}

			outStaticMeshes[StaticMeshIndex].SetTextureSupport(HasTextures);

			for (uint32 j = 0; j < rawData[i].WorldMatrices.size(); ++j)
			{
				ShaderSceneData->WorldMatrices[j + WorldMatricesOffset] = rawData[i].WorldMatrices[j];
			}

			for (uint32 j = 0; j < rawData[i].MaterialCount; ++j)
			{
				Material Mat;
				Mat.Diffuse = reinterpret_cast<Vector3D&>(rawData[i].Materials[j].attrib.Kd);
				Mat.Dissolve = rawData[i].Materials[j].attrib.d;
				Mat.SpecularColor = reinterpret_cast<Vector3D&>(rawData[i].Materials[j].attrib.Ks);
				Mat.SpecularExponent = rawData[i].Materials[j].attrib.Ns;
				Mat.Ambient = reinterpret_cast<Vector3D&>(rawData[i].Materials[j].attrib.Ka);
				Mat.Sharpness = rawData[i].Materials[j].attrib.sharpness;
				Mat.TransmissionFilter = reinterpret_cast<Vector3D&>(rawData[i].Materials[j].attrib.Tf);
				Mat.OpticalDensity = rawData[i].Materials[j].attrib.Ni;
				Mat.Emissive = reinterpret_cast<Vector3D&>(rawData[i].Materials[j].attrib.Ke);
				Mat.IlluminationModel = rawData[i].Materials[j].attrib.illum;

				ShaderSceneData->Materials[j + MaterialOffset] = Mat;
			}

			MaterialOffset += rawData[i].MaterialCount;
			TextureOffset += TexOffsetPerMesh;
			WorldMatricesOffset += rawData[i].WorldMatrices.size();

			StaticMeshIndex++;
		}

		/* Load all textures */
		Textures.resize(TexturePaths.size());
		for (uint32 i = 0; i < TexturePaths.size(); ++i)
		{
			CreateDefaultSampler(Textures[i].Texture.levelCount, Textures[i].Sampler);
			LoadTexture(TexturePaths[i].c_str(), Textures[i]);
			CreateDefaultImageViewFromTexture(&Textures[i], Textures[i].View);
		}
	}
};
