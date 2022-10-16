#pragma once
#include <string>
#include <vector>
#include "h2bParser.h"
#include "Math/Matrix4D.h"

#include "GenericDefines.h"

struct RawMaterial
{
	H2B::ATTRIBUTES attrib;

	std::string name;
	std::string map_Kd; // Diffuse map
	std::string map_Ks; // roughness map
	std::string map_Ka; // ambient map
	std::string map_Ke; // emissive map
	std::string map_Ns; // specular 
	std::string map_d; // dissolve map
	std::string disp;
	std::string decal;
	std::string bump; // Normal map
	const void* padding[2];
};

enum LightType
{
	Directional =	0,
	Point =			1,
	Spot =			2
};

struct RawMeshData
{
	std::string Name;

	/* Mesh Info */
	uint32 VertexCount;
	uint32 IndexCount;
	uint32 MaterialCount;
	uint32 MeshCount;

	std::vector<H2B::VERTEX> Vertices;
	std::vector<uint32> Indices;
	std::vector<RawMaterial> Materials;
	std::vector<H2B::BATCH> Batches;
	std::vector<H2B::MESH> Meshes;

	std::vector<Matrix4D> WorldMatrices;
	uint32 InstanceCount;

	bool IsCamera;
	bool IsLight;

	LightType Light;

	RawMeshData()
	{
		VertexCount = 0;
		IndexCount = 0;
		MaterialCount = 0;
		MeshCount = 0;
		InstanceCount = 0;

		IsCamera = false;
		IsLight = false;

		Light = LightType::Point;
	}
};

/* 
* Sub Mesh  -> Can think of this as that 
*		Name -> name of the mesh
*		Index Offset -> index offset of the meshes indices block
*		Index Count -> amount of indices to draw 
*		Material Index -> the material associated with this mesh
*/
struct Mesh
{
	std::string Name;

	uint32 IndexOffset;
	uint32 IndexCount;

	uint32 MaterialIndex;

	uint32 DiffuseTextureIndex;

	Mesh()
	{
		Name = "Unnamed";
		IndexOffset = 0;
		IndexCount = 0;
		MaterialIndex = 0;
		DiffuseTextureIndex = 0;
	}
};

/*
* A Collection of Meshes in one
*		IndexOffset -> index offset into the collection of meshes indices
*		MaterialIndex -> Index of the first material into the collection of materials 
*		WorldMatrixIndex -> index of the first world matrix into the collection of world matrices
*/
struct StaticMesh
{
	uint32 VertexCount;
	uint32 IndexCount;
	uint32 MaterialCount;
	uint32 MeshCount;
	uint32 InstanceCount;

	uint32 VertexOffset;
	uint32 IndexOffset;
	uint32 MaterialIndex;
	uint32 WorldMatrixIndex;

	std::vector<Mesh> Meshes;
};

struct PointLight
{
	/* W component is used for strength of the point light */
	Vector4D Position;

	/* W component is used for attenuation */
	Vector4D Color;

	void AddStrength(float strength)
	{
		Position.W += strength;
	}

	void SetRadius(float radius)
	{
		Color.W = radius;
	}
};

struct SpotLight
{
	/* W Component - spot light strength */
	Vector4D Position;

	/* W Component - cone ratio */
	Vector4D Color;
	Vector4D ConeDirection;

	void AddStrength(float strength)
	{
		Position.W += strength;
	}

	void SetConeRatio(float ratio)
	{
		Color.W = ratio;
	}
};