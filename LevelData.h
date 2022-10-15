#pragma once
#include "Math/Vector4D.h"
#include "Math/Vector2D.h"
#include <vector>
#include "Math/Matrix4D.h"
#include "RawMeshData.h"
#include "GenericDefines.h"
#include "FileHelper.h"
#include "GatewareDefine.h"

#include "FSLogo.h"

struct Vertex
{
	Vector2D TexCoord;
	Vector3D Position;
	Vector3D Normal;
	Vector4D Color;
};

struct Material
{
	Vector3D	Diffuse;
	float		Dissolve; // Transparency
	Vector3D	SpecularColor;
	float		SpecularExponent;
	Vector3D	Ambient;
	float		Sharpness;
	Vector3D	TransmissionFilter;
	float		OpticalDensity;
	Vector3D	Emissive;
	uint32		IlluminationModel;
};

class LevelData
{
private:
	std::vector<Vertex> Vertices;
	std::vector<uint32> Indices;
	std::vector<Matrix4D> WorldMatrices;

	uint32 TotalVertices;
	uint32 TotalIndices;

	/* Rendering Vars */
	GW::GRAPHICS::GVulkanSurface* VlkSurface;

	VkDevice* Device = nullptr;

	/* Vertex/Index Buffers */
	VkBuffer VertexBufferHandle = nullptr;
	VkDeviceMemory VertexBufferData = nullptr;

	VkBuffer IndexBufferHandle = nullptr;
	VkDeviceMemory IndexBufferData = nullptr;

public:
	LevelData(VkDevice* deviceHandle, GW::GRAPHICS::GVulkanSurface* vlkSurface, std::vector<RawMeshData>& rawMeshDatas)
	{
		Device = deviceHandle;
		VlkSurface = vlkSurface;

		TotalIndices = 0;
		TotalVertices = 0;

		for (uint32 i = 0; i < rawMeshDatas.size(); ++i)
		{
			if (rawMeshDatas[i].IsCamera)
			{
				continue;
			}

			AddRawMeshData(rawMeshDatas[i]);
		}
	}

	LevelData() = delete;
	LevelData(const LevelData& Other) = delete;

	~LevelData()
	{
		if (VertexBufferHandle)
		{
			vkDestroyBuffer(*Device, VertexBufferHandle, nullptr);
			vkFreeMemory(*Device, VertexBufferData, nullptr);
		}
		
		if (IndexBufferHandle)
		{
			vkDestroyBuffer(*Device, IndexBufferHandle, nullptr);
			vkFreeMemory(*Device, IndexBufferData, nullptr);
		}		
	}

public:
	/* Bind Data to API */
	void Bind()
	{
		uint32 CurrentBuffer = 0;
		VlkSurface->GetSwapchainCurrentImage(CurrentBuffer);

		VkCommandBuffer CommandBuffer;
		VlkSurface->GetCommandBuffer(CurrentBuffer, (void**)&CommandBuffer);

		VkDeviceSize Offsets[] = { 0 };
		
		/* Bind Vertex and Index Buffers */
		vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertexBufferHandle, Offsets);
		vkCmdBindIndexBuffer(CommandBuffer, IndexBufferHandle, Offsets[0], VK_INDEX_TYPE_UINT32);
	}

	void Load()
	{
		LoadVertexAndIndexData();
	}

	void Unload()
	{

	}

public:
	void AddRawMeshData(RawMeshData& rawMeshData)
	{
		/* Copy All Vertices */
		TotalVertices += rawMeshData.VertexCount;
		Vertex V;
		V.Color = Vector4D(0.5f, 0.5f, 0.5f, 1.0f);
		for (uint32 i = 0; i < rawMeshData.VertexCount; ++i)
		{
			V.TexCoord = Vector2D(rawMeshData.Vertices[i].uvw.x, rawMeshData.Vertices[i].uvw.y);
			V.Position = Vector3D(rawMeshData.Vertices[i].pos.x, rawMeshData.Vertices[i].pos.y, rawMeshData.Vertices[i].pos.z);
			V.Normal = Vector3D(rawMeshData.Vertices[i].nrm.x, rawMeshData.Vertices[i].nrm.y, rawMeshData.Vertices[i].nrm.z);
			Vertices.push_back(V);
		}

		/* Copy All Indices */
		TotalIndices += rawMeshData.IndexCount;
		for (uint32 i = 0; i < rawMeshData.IndexCount; ++i)
		{
			Indices.push_back(rawMeshData.Indices[i]);
		}

		/* Copy All WorldMatrices */
		for (uint32 i = 0; i < rawMeshData.WorldMatrices.size(); ++i)
		{
			WorldMatrices.push_back(rawMeshData.WorldMatrices[i]);
		}
	}

private:
	void LoadVertexData()
	{
		GW::GReturn Result;
		VkPhysicalDevice PhysicalDevice = nullptr;
		if ((Result = VlkSurface->GetPhysicalDevice((void**)&PhysicalDevice)) != GW::GReturn::SUCCESS)
		{
			std::cout << "\n[Gateware] Failed to get PhysicalDevice....";
			return;
		}

		uint32 VerticesSizeInBytes = sizeof(Vertex) * Vertices.size();

		GvkHelper::create_buffer(PhysicalDevice, *Device, VerticesSizeInBytes,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &VertexBufferHandle, &VertexBufferData);
		GvkHelper::write_to_buffer(*Device, VertexBufferData, Vertices.data(), VerticesSizeInBytes);
	}
	
	void LoadIndexData()
	{
		GW::GReturn Result;
		VkPhysicalDevice PhysicalDevice = nullptr;
		if ((Result = VlkSurface->GetPhysicalDevice((void**)&PhysicalDevice)) != GW::GReturn::SUCCESS)
		{
			std::cout << "\n[Gateware] Failed to get PhysicalDevice....";
			return;
		}

		uint32 IndicesSizeInBytes = Indices.size() * sizeof(uint32);

		GvkHelper::create_buffer(PhysicalDevice, *Device, IndicesSizeInBytes,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &IndexBufferHandle, &IndexBufferData);
		GvkHelper::write_to_buffer(*Device, IndexBufferData, Indices.data(), IndicesSizeInBytes);
	}

	void LoadVertexAndIndexData()
	{
		GW::GReturn Result;
		VkPhysicalDevice PhysicalDevice = nullptr;
		if ((Result = VlkSurface->GetPhysicalDevice((void**)&PhysicalDevice)) != GW::GReturn::SUCCESS)
		{
			std::cout << "\n[Gateware] Failed to get PhysicalDevice....";
			return;
		}

		uint32 IndicesSizeInBytes = Indices.size() * sizeof(uint32);
		uint32 VerticesSizeInBytes = sizeof(Vertex) * Vertices.size();

		GvkHelper::create_buffer(PhysicalDevice, *Device, VerticesSizeInBytes,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &VertexBufferHandle, &VertexBufferData);
		GvkHelper::write_to_buffer(*Device, VertexBufferData, Vertices.data(), VerticesSizeInBytes);

		GvkHelper::create_buffer(PhysicalDevice, *Device, IndicesSizeInBytes,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &IndexBufferHandle, &IndexBufferData);
		GvkHelper::write_to_buffer(*Device, IndexBufferData, Indices.data(), IndicesSizeInBytes);
	}
};
