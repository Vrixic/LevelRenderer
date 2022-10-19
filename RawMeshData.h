#pragma once
#include <string>
#include "h2bParser.h"
#include "Math/Matrix4D.h"

#include "GenericDefines.h"

struct RawMaterial
{
	H2B::ATTRIBUTES attrib;
	std::string name;
	std::string DiffuseMap; // Diffuse map - map_Kd
	std::string RoughnessMap; // roughness map - map_Ks
	std::string AmbientMap; // ambient map - map_Ka
	std::string EmissiveMap; // emissive map - map_Ke
	std::string SpecularMap; // specular  - map_Ns
	std::string DissolveMap; // dissolve map - map_d
	std::string disp;
	std::string decal;
	std::string NormalMap; // Normal map - bump
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

	Vector3D BoxMin_AABB;
	Vector3D BoxMax_AABB;

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

		BoxMin_AABB = Vector3D::ZeroVector();
		BoxMax_AABB = Vector3D::ZeroVector();
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

	int32 DiffuseTextureIndex;
	int32 SpecularTextureIndex;

	Mesh()
	{
		Name = "Unnamed";
		IndexOffset = 0;
		IndexCount = 0;
		MaterialIndex = 0;
		DiffuseTextureIndex = 0;
	}
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

	/* W Component - inner cone ratio */
	Vector4D Color;

	/* W component - outer cone ratio*/
	Vector4D ConeDirection;

	void AddStrength(float strength)
	{
		Position.W += strength;
	}

	void SetInnerConeRatio(float ratio)
	{
		Color.W = ratio;
	}

	void SetOuterConeRatio(float ratio)
	{
		ConeDirection.W = ratio;
	}
};