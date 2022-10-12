#include "FileHelper.h"
#include <iostream>

std::string FileHelper::LoadShaderFileIntoString(const char* shaderFilePath)
{
	std::ifstream FileHandle(shaderFilePath);
	if (!FileHandle.is_open())
	{
#if NDEBUG
		std::cout << "ERROR: Shader Source File \"" << shaderFilePath << "\" Not Found!" << std::endl;
#endif // DEBUG
		return "";
	}

	FileHandle.seekg(0, std::ios::end);
	size_t Size = FileHandle.tellg();
	std::string Output(Size, ' ');
	FileHandle.seekg(0);
	FileHandle.read(&Output[0], Size);
	return Output;
}

void FileHelper::ReadGameLevelFile(const char* path, std::vector<RawMeshData>& Meshes)
{
	std::ifstream FileHandle(path);
	std::string LastMeshName = "";
	std::string Line = "";
	Matrix4D Matrix;
	unsigned int MeshIndex = -1;

	H2B::Parser Parser;

#if NDEBUG
	std::cout << "[FILE] Opening GameLevel.txt\n";
#endif // DEBUG
	if (FileHandle.is_open())
	{
#if NDEBUG
		std::cout << "[FILE] Opened GameLevel.txt\n";
#endif // DEBUG
		while (true)
		{
			std::getline(FileHandle, Line);

			if (FileHandle.eof()) { break; }

			if (std::strcmp(Line.c_str(), "MESH") == 0)
			{
				/* Read Mesh Infomation */
				std::getline(FileHandle, Line, '\n');

				/* Mesh name
				* Better to go backwards maybe, since we only have to look for the peroid
				* if it exists, if not only one instance exists for the current Mesh
				* else there are more instances of the same mesh
				*/
				bool bIsInstance = false;
				std::string CurrentMeshName = Line;
				for (unsigned int i = Line.length() - 1; i > 0; --i)
				{
					if (Line[i] == '.') // an insance of current mesh or new mesh
					{
						bIsInstance = true;
						break;
					}

					Line.pop_back();
				}

				Line.pop_back(); // Account for a period or one character 

				if (!bIsInstance || std::strcmp(LastMeshName.c_str(), Line.c_str()) != 0)
				{
					bool bNewMesh = true;
					LastMeshName = Line.length() == 0 ? CurrentMeshName : Line;

					// Check if the mesh was previously added
					for (unsigned int i = 0; i < Meshes.size(); ++i)
					{
						if (std::strcmp(Meshes[i].Name.c_str(), LastMeshName.c_str()) == 0)
						{
							bNewMesh = false;
							MeshIndex = i;
							break;
						}
					}

					/* New mesh */
					if (bNewMesh)
					{
#if NDEBUG
						std::cout << "[Mesh Found]: " << LastMeshName << "\n";
#endif // DEBUG

						RawMeshData EmptyMesh;
						EmptyMesh.Name = LastMeshName;
						EmptyMesh.InstanceCount = 0;

						if (!FillRawMeshDataFromH2BFile(("../Assets/h2b/" + LastMeshName + ".h2b").c_str(), Parser, EmptyMesh))
						{
#if NDEBUG
							std::cout << "[Error]: Mesh h2b file could not be read or found....\n";
#endif // DEBUG
						}

						Meshes.push_back(EmptyMesh);
						MeshIndex = Meshes.size() - 1;
					}
				}

				Meshes[MeshIndex].InstanceCount++;
#if NDEBUG
				std::cout << "[Mesh Instance]: " << LastMeshName << ": " << Meshes[MeshIndex].InstanceCount << "\n";
#endif // DEBUG

				/* Read the Matrix */
				ReadMatrixFromFile(FileHandle, Matrix, Line);
#if NDEBUG
				std::cout << Line;
#endif // DEBUG

				Meshes[MeshIndex].WorldMatrices.push_back(Matrix);
			}
		}

		FileHandle.close();
	}
	else
	{
#if NDEBUG
		std::cout << "[FILE] Error could not open file..." << std::endl;
#endif // DEBUG
	}
}

void FileHelper::ReadMatrixFromFile(std::ifstream& fileHandle, Matrix4D& outMatrix, std::string& outMatrixString)
{
	outMatrixString.clear();

	std::string Line = "";
	std::getline(fileHandle, Line, '('); // Read Matrix Header
	outMatrixString.append(Line + "(");

	std::getline(fileHandle, Line, ')'); // read row 1
	outMatrixString.append(Line + ")");
	Vector4D V1 = ReadVectorFromString(Line);

	std::getline(fileHandle, Line, ')'); // read row 2
	outMatrixString.append(Line + ")");
	Vector4D V2 = ReadVectorFromString(Line);

	std::getline(fileHandle, Line, ')'); // read row 3
	outMatrixString.append(Line + ")");
	Vector4D V3 = ReadVectorFromString(Line);

	std::getline(fileHandle, Line, ')'); // read row 4
	outMatrixString.append(Line + ")>\n\n");
	Vector4D V4 = ReadVectorFromString(Line);

	outMatrix = Matrix4D(V1, V2, V3, V4);
}

bool FileHelper::FillRawMeshDataFromH2BFile(const char* filePath, H2B::Parser& parser, RawMeshData& outMesh)
{
	if (parser.Parse(filePath))
	{
		outMesh.VertexCount = parser.vertexCount;
		outMesh.IndexCount = parser.indexCount;
		outMesh.MaterialCount = parser.materialCount;
		outMesh.MeshCount = parser.meshCount;
		outMesh.Vertices = parser.vertices;
		outMesh.Indices = parser.indices;
		outMesh.Materials = parser.materials;
		outMesh.Batches = parser.batches;
		outMesh.Meshes = parser.meshes;

#if NDEBUG
		std::cout << "[H2B]: Successfully parsed file" << " filePath" << "....\n";
#endif // DEBUG

		return true;
	}

	return false;
}

Vector4D FileHelper::ReadVectorFromString(std::string& str)
{
	Vector4D V = {};
	unsigned int VIndex = 0;
	std::string Num = "";

	for (unsigned int i = 0; i < str.length(); ++i)
	{
		unsigned int AcsiiChar = static_cast<unsigned int>(str[i]);
		if (str[i] == ',')
		{
			switch (VIndex)
			{
			case 0:
				V.X = std::stof(Num.c_str());
				break;
			case 1:
				V.Y = std::stof(Num.c_str());
				break;
			case 2:
				V.Z = std::stof(Num.c_str());
				break;
			}

			VIndex++;
			Num.clear();
		}
		else if ((AcsiiChar >= 45 && AcsiiChar <= 57) && AcsiiChar != 47)
		{
			Num.push_back(str[i]);
		}
	}

	V.W = std::stof(Num.c_str());

	return V;
}
