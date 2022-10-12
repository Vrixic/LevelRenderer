#pragma once
#include <string>
#include <vector>
#include "h2bParser.h"
#include "Math/Matrix4D.h"

struct RawMeshData
{
	std::string Name;

	/* Mesh Info */
	unsigned int VertexCount;
	unsigned int IndexCount;
	unsigned int MaterialCount;
	unsigned int MeshCount;

	std::vector<H2B::VERTEX> Vertices;
	std::vector<unsigned int> Indices;
	std::vector<H2B::MATERIAL> Materials;
	std::vector<H2B::BATCH> Batches;
	std::vector<H2B::MESH> Meshes;

	std::vector<Matrix4D> WorldMatrices;
	unsigned int InstanceCount;
};