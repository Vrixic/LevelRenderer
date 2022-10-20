#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "RawMeshData.h"

#define DEBUG 1

struct FileHelper
{
public:

	static bool OpenFileDialog(std::string& outFilePath);

	// Load a shader file as a string of characters.
	static std::string LoadShaderFileIntoString(const char* shaderFilePath);

	static void ReadGameLevelFile(const char* path, std::vector<RawMeshData>& Meshes);

	static bool FillRawMeshDataFromH2BFile(const char* filePath, H2B::Parser* parser, RawMeshData& outMesh);

	static void ParseFileNameFromPath(const std::string& inPath, std::string& outFileName);

private:

	static Vector4D ReadVectorFromString(std::string& str);

	static void ReadMatrixFromFile(std::ifstream& fileHandle, Matrix4D& outMatrix, std::string& outMatrixString);
};

