#include "FileHelper.h"
#include <iostream>
#include <unordered_map>

#include <windows.h>
#include <ShObjIdl.h>

#include <sstream>

bool FileHelper::OpenFileDialog(std::string& outFilePath)
{
	HRESULT Result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
		COINIT_DISABLE_OLE1DDE);
	IFileOpenDialog* FileOpenDialog;

	// Create the dialog object
	Result = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
		IID_IFileOpenDialog, reinterpret_cast<void**>(&FileOpenDialog));

	if (SUCCEEDED(Result))
	{
		// Show the open dialog box
		Result = FileOpenDialog->Show(NULL);

		// Get the file name from the dialog box
		if (SUCCEEDED(Result))
		{
			IShellItem* ShellItem;
			Result = FileOpenDialog->GetResult(&ShellItem);

			if (SUCCEEDED(Result))
			{
				PWSTR FilePath;
				Result = ShellItem->GetDisplayName(SIGDN_FILESYSPATH, &FilePath);

				// convert pointer to wide string to char* to std::string
				std::wstring WideFilePath = FilePath;
				outFilePath = std::string(WideFilePath.begin(), WideFilePath.end());
				return true;
			}
		}
	}

	return false;
}

std::string FileHelper::LoadShaderFileIntoString(const char* shaderFilePath)
{
	std::ifstream FileHandle(shaderFilePath);
	if (!FileHandle.is_open())
	{
#if DEBUG
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

	std::unordered_map<std::string, uint32> InstancingMap;

	H2B::Parser* Parser = new H2B::Parser;

#if DEBUG
	std::cout << "[FILE] Opening " << path << "\n";
#endif // DEBUG
	if (FileHandle.is_open())
	{
#if DEBUG
		std::cout << "[FILE] Opened " << path << "\n\n";
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
				std::string CurrentMeshName = Line;
				for (unsigned int i = Line.length() - 1; i > 0; --i)
				{
					if (Line[i] == '.') // an insance of current mesh or new mesh
					{
						break;
					}
					Line.pop_back();
				}

				Line.pop_back(); // Account for a period or one character 

				LastMeshName = Line.length() == 0 ? CurrentMeshName : Line;
				std::unordered_map<std::string, uint32>::iterator It = InstancingMap.find(LastMeshName);

				if (It == InstancingMap.end())
				{
					/* New mesh */
#if DEBUG
					std::cout << "[Mesh Found]: " << LastMeshName << "\n";
#endif // DEBUG					

					RawMeshData EmptyMesh;
					EmptyMesh.Name = LastMeshName;
					EmptyMesh.InstanceCount = 0;
					EmptyMesh.IsCamera = false;

					if (!FillRawMeshDataFromH2BFile(("../Assets/h2b/" + LastMeshName + ".h2b").c_str(), Parser, EmptyMesh))
					{
#if DEBUG
						std::cout << "[Error]: Mesh h2b file could not be read or found....\n";
#endif // DEBUG
					}


					Meshes.push_back(EmptyMesh);
					MeshIndex = Meshes.size() - 1;

					InstancingMap.insert(std::pair<std::string, uint32>(LastMeshName, MeshIndex));
				}
				else
				{
					MeshIndex = It->second;
				}
				Meshes[MeshIndex].InstanceCount++;
#if DEBUG
				std::cout << "[Mesh Instance]: " << LastMeshName << ": " << Meshes[MeshIndex].InstanceCount << "\n";
#endif // DEBUG

				/* Read the Matrix */
				ReadMatrixFromFile(FileHandle, Matrix, Line);

#if DEBUG
				std::cout << Line;
#endif // DEBUG

				Meshes[MeshIndex].WorldMatrices.push_back(Matrix);
			}
			else if (std::strcmp(Line.c_str(), "CAMERA") == 0)
			{
				if (InstancingMap.find("CAMERA") == InstancingMap.end())
				{
					std::getline(FileHandle, Line, '\n');
#if DEBUG
					std::cout << "[Camera Found]: " << Line << "\n";
#endif // DEBUG
					RawMeshData EmptyMesh;
					EmptyMesh.Name = Line;
					EmptyMesh.IsCamera = true;

					Meshes.push_back(EmptyMesh);
					MeshIndex = Meshes.size() - 1;
					InstancingMap.insert(std::pair<std::string, uint32>("CAMERA", MeshIndex));
				}
				else
				{
					std::getline(FileHandle, Line, '\n');
#if DEBUG
					std::cout << "[Camera Found]: " << Line << ".. will be discarded, multiple cameras not supported..\n";
#endif // DEBUG
				}

				/* Read the Matrix */
				ReadMatrixFromFile(FileHandle, Matrix, Line);
#if DEBUG
				std::cout << Line;
#endif // DEBUG

				Meshes[MeshIndex].WorldMatrices.push_back(Matrix);
			}
			else if (std::strcmp(Line.c_str(), "LIGHT") == 0)
			{
				std::getline(FileHandle, Line, '\n');

#if DEBUG
				std::cout << "[Light Found]: " << Line << "\n";
#endif // DEBUG
				std::string LineCopy = Line;
				for (unsigned int i = Line.length() - 1; i > 0; --i)
				{
					if (Line[i] == '.') // an insance of current mesh or new mesh
					{
						break;
					}

					Line.pop_back();
				}
				Line.pop_back(); // Account for a period or one character 

				LightType LType;

				Line = Line.length() == 0 ? LineCopy : Line;

				//				if (std::strcmp(Line.c_str(), "Point") != 0)
				//				{
				//#if DEBUG
				//					std::cout << "[FileHelper]: A light was found and discarded...\n";
				//
				//					std::getline(FileHandle, Line, '>');
				//					std::cout << Line << ">";
				//					std::getline(FileHandle, Line);
				//					std::cout << Line;
				//#endif // DEBUG
				//				}				

				RawMeshData EmptyMesh;
				EmptyMesh.Name = Line;
				EmptyMesh.IsCamera = false;
				EmptyMesh.IsLight = true;

				if (std::strcmp(Line.c_str(), "Point") == 0)
				{
					EmptyMesh.Light = LightType::Point;
				}
				else if (std::strcmp(Line.c_str(), "Sun") == 0)
				{
					EmptyMesh.Light = LightType::Directional;
				}
				else if (std::strcmp(Line.c_str(), "Spot") == 0)
				{
					EmptyMesh.Light = LightType::Spot;
				}

				Meshes.push_back(EmptyMesh);
				MeshIndex = Meshes.size() - 1;

				/* Read the Matrix */
				ReadMatrixFromFile(FileHandle, Matrix, Line);
#if DEBUG
				std::cout << Line;
#endif // DEBUG

				Meshes[MeshIndex].WorldMatrices.push_back(Matrix);
			}
		}

		/*--------------------------------------------------DEBUG-------------------------------------------------------*/

		/*std::cout << "[Materials]: \n";
		for (uint32 i = 0; i < Meshes.size(); ++i)
		{
			for (uint32 j = 0; j < Meshes[i].MaterialCount; ++j)
			{
				if (Meshes[i].Materials[j].map_Kd.c_str() == nullptr)
				{
					std::cout << "[Error]: no string or bad string: at index: " << j << std::endl;
					continue;
				}
				std::cout << Meshes[i].Materials[j].map_Kd << "\n";
			}
		}
		std::cout << "\n";*/

		/*--------------------------------------------------DEBUG-------------------------------------------------------*/

		delete Parser;
		FileHandle.close();
	}
	else
	{
#if DEBUG
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
	outMatrixString.append(Line + ")\n\n");
	Vector4D V4 = ReadVectorFromString(Line);

	std::getline(fileHandle, Line, '\n');

	outMatrix = Matrix4D(V1, V2, V3, V4);
}

void FileHelper::ParseFileNameFromPath(const std::string& inPath, std::string& outFileName)
{
	for (uint32 i = 0; i < inPath.length(); ++i)
	{
		if (inPath[i] == '\\' || inPath[i] == '/')
		{
			break;
		}

		outFileName.append(&inPath[i]);
	}
}

bool FileHelper::FillRawMeshDataFromH2BFile(const char* filePath, H2B::Parser* parser, RawMeshData& outMesh)
{
	if (parser->Parse(filePath))
	{
		outMesh.VertexCount = parser->vertexCount;
		outMesh.IndexCount = parser->indexCount;
		outMesh.MaterialCount = parser->materialCount;
		outMesh.MeshCount = parser->meshCount;
		outMesh.Vertices = parser->vertices;
		outMesh.Indices = parser->indices;

		float MinX = FLT_MAX;
		float MinY = FLT_MAX;
		float MinZ = FLT_MAX;

		float MaxX = FLT_MIN;
		float MaxY = FLT_MIN;
		float MaxZ = FLT_MIN;

		/* Aabb*/
		for (uint32 i = 0; i < parser->vertexCount; ++i)
		{
			H2B::VECTOR Position = parser->vertices[i].pos;

			if (Position.x < MinX)
			{
				MinX = Position.x;
			}
			if (Position.y < MinY)
			{
				MinY = Position.y;
			}
			if (Position.z < MinZ)
			{
				MinZ = Position.z;
			}

			if (Position.x > MaxX)
			{
				MaxX = Position.x;
			}
			if (Position.y > MaxY)
			{
				MaxY = Position.y;
			}
			if (Position.z > MaxZ)
			{
				MaxZ = Position.z;
			}
		}

		outMesh.BoxMin_AABB = Vector3D(MinX, MinY, MinZ);
		outMesh.BoxMax_AABB = Vector3D(MaxX, MaxY, MaxZ);

		for (uint32 i = 0; i < parser->materialCount; ++i)
		{
			RawMaterial Mat;
			Mat.attrib = parser->materials[i].attrib;
			Mat.DiffuseMap = std::string(parser->materials[i].map_Kd == nullptr ? "" : parser->materials[i].map_Kd);
			Mat.SpecularMap = std::string(parser->materials[i].map_Ks == nullptr ? "" : parser->materials[i].map_Ks);
			outMesh.Materials.push_back(Mat);
		}

		outMesh.Batches = parser->batches;
		outMesh.Meshes = parser->meshes;

#if DEBUG
		std::cout << "[H2B]: Successfully parsed file" << filePath << "....\n";
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
