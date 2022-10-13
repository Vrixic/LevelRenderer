#pragma once
#include <string>
#include <vector>
#include "h2bParser.h"
#include "Math/Matrix4D.h"

#include "GenericDefines.h"

struct RawMeshData
{
	std::string Name;

	/* Mesh Info */
	uint32 VertexCount;
	uint32 IndexCount;
	uint32 MaterialCount;
	uint32 MeshCount;

	std::vector<H2B::VERTEX> Vertices;
	std::vector<unsigned int> Indices;
	std::vector<H2B::MATERIAL> Materials;
	std::vector<H2B::BATCH> Batches;
	std::vector<H2B::MESH> Meshes;

	std::vector<Matrix4D> WorldMatrices;
	uint32 InstanceCount;
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

	uint32 IndexOffset;
	uint32 MaterialIndex;
	uint32 WorldMatrixIndex;

	std::vector<Mesh> Meshes;
};