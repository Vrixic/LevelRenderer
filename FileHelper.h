#pragma once
#include <string>
#include <vector>
#include "RawMeshData.h"

struct FileHelper
{
public:

	// Load a shader file as a string of characters.
	static std::string LoadShaderFileIntoString(const char* shaderFilePath);

	static void ReadGameLevelFile(const char* path, std::vector<RawMeshData>& Meshes);

	static bool FillRawMeshDataFromH2BFile(const char* filePath, H2B::Parser& parser, RawMeshData& outMesh);

private:

	static Vector4D ReadVectorFromString(std::string& str);

	static void ReadMatrixFromFile(std::ifstream& fileHandle, Matrix4D& outMatrix, std::string& outMatrixString);
};

