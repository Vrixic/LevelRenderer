// Simple basecode showing how to create a window and attatch a vulkansurface
#define GATEWARE_ENABLE_CORE // All libraries need this
#define GATEWARE_ENABLE_SYSTEM // Graphics libs require system level libraries
#define GATEWARE_ENABLE_GRAPHICS // Enables all Graphics Libraries
// Ignore some GRAPHICS libraries we aren't going to use
#define GATEWARE_DISABLE_GDIRECTX11SURFACE // we have another template for this
#define GATEWARE_DISABLE_GDIRECTX12SURFACE // we have another template for this
#define GATEWARE_DISABLE_GRASTERSURFACE // we have another template for this
#define GATEWARE_DISABLE_GOPENGLSURFACE // we have another template for this
// TODO: Part 2a
#define GATEWARE_ENABLE_MATH
// TODO: Part 4a
#define GATEWARE_ENABLE_INPUT
// With what we want & what we don't defined we can include the API
#include "Gateware.h"
#include "renderer.h"
#include "h2bParser.h"
#include <fstream>

// open some namespaces to compact the code a bit
using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;


struct StaticMesh
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

	std::vector<GW::MATH::GMATRIXF> WorldMatrices;
	unsigned int InstanceCount;
};

GW::MATH::GVECTORF ReadVectorFromString(std::string& str)
{
	GW::MATH::GVECTORF V = {};
	unsigned int VIndex = 0;
	std::string Num = "";

	for (unsigned int i = 0; i < str.length() - 1; ++i)
	{
		unsigned int AcsiiChar = static_cast<unsigned int>(str[i]);
		if (str[i] == ',')
		{
			V.data[VIndex] = std::stof(Num.c_str());
			VIndex++;
			Num.clear();
		}
		else if ((AcsiiChar >= 45 && AcsiiChar <= 57) && AcsiiChar != 47)
		{
			Num.push_back(str[i]);
		}

	}

	return V;
}

bool FillStaticMeshFromH2BFile(const char* filePath, H2B::Parser& parser, StaticMesh& outMesh)
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

		std::cout << "[H2B]: Successfully parsed file" << " filePath" << "....\n";

		return true;
	}

	return false;
}

void ReadMatrixFromFile(std::ifstream& fileHandle, GW::MATH::GMATRIXF& outMatrix, std::string& outMatrixString)
{
	outMatrixString.clear();

	std::string Line = "";
	std::getline(fileHandle, Line, '('); // Read Matrix Header
	outMatrixString.append(Line + "("); 

	std::getline(fileHandle, Line, ')'); // read row 1
	outMatrixString.append(Line + ")");
	outMatrix.row1 = ReadVectorFromString(Line);

	std::getline(fileHandle, Line, ')'); // read row 2
	outMatrixString.append(Line + ")");
	outMatrix.row2 = ReadVectorFromString(Line);

	std::getline(fileHandle, Line, ')'); // read row 3
	outMatrixString.append(Line + ")");
	outMatrix.row3 = ReadVectorFromString(Line);

	std::getline(fileHandle, Line, ')'); // read row 4
	outMatrixString.append(Line + ")>\n\n");
	outMatrix.row4 = ReadVectorFromString(Line);
}

void ReadGameLevelFile(std::vector<StaticMesh>& Meshes)
{
	std::ifstream FileHandle("../GameLevel.txt");
	std::string LastMeshName = "";
	std::string Line = "";
	GW::MATH::GMATRIXF Matrix;
	unsigned int MeshIndex = -1;

	H2B::Parser Parser;

	std::cout << "[FILE] Opening GameLevel.txt\n";
	if (FileHandle.is_open())
	{
		std::cout << "[FILE] Opened GameLevel.txt\n";
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
						std::cout << "[Mesh Found]: " << LastMeshName << "\n";

						StaticMesh EmptyMesh;
						EmptyMesh.Name = LastMeshName;
						EmptyMesh.InstanceCount = 0;

						if (!FillStaticMeshFromH2BFile(("../Assets/h2b/" + LastMeshName + ".h2b").c_str(), Parser, EmptyMesh))
						{
							std::cout << "[Error]: Mesh h2b file could not be read or found....\n";
						}

						Meshes.push_back(EmptyMesh);
						MeshIndex = Meshes.size() - 1;
					}
				}

				Meshes[MeshIndex].InstanceCount++;
				std::cout << "[Mesh Instance]: " << LastMeshName << ": " << Meshes[MeshIndex].InstanceCount << "\n";

				/* Read the Matrix */
				ReadMatrixFromFile(FileHandle, Matrix, Line);
				std::cout << Line;

				Meshes[MeshIndex].WorldMatrices.push_back(Matrix);
			}
		}

		FileHandle.close();
	}
	else
	{
		std::cout << "[FILE] Error could not open file..." << std::endl;
	}
}

// lets pop a window and use Vulkan to clear to a red screen
int main()
{
	GWindow win;
	GEventResponder msgs;
	GVulkanSurface vulkan;

	std::vector<StaticMesh> Meshes;
	ReadGameLevelFile(Meshes);

	if (+win.Create(0, 0, 800, 600, GWindowStyle::WINDOWEDBORDERED))
	{
		// TODO: Part 1a
		win.SetWindowName("Vrij Patel + Assignment 1 - Vulkan");
		VkClearValue clrAndDepth[2];
		clrAndDepth[0].color = { {0.0f, 0.0f, 0.0f, 1} }; // TODO: Part 1a
		clrAndDepth[1].depthStencil = { 1.0f, 0u };
		msgs.Create([&](const GW::GEvent& e) {
			GW::SYSTEM::GWindow::Events q;
			if (+e.Read(q) && q == GWindow::Events::RESIZE)
				clrAndDepth[0].color.float32[2] += 0.01f; // disable
			});
		win.Register(msgs);
#ifndef NDEBUG
		const char* debugLayers[] = {
			"VK_LAYER_KHRONOS_validation", // standard validation layer
			//"VK_LAYER_LUNARG_standard_validation", // add if not on MacOS
			//"VK_LAYER_RENDERDOC_Capture" // add this if you have installed RenderDoc
		};
		if (+vulkan.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT,
			sizeof(debugLayers) / sizeof(debugLayers[0]),
			debugLayers, 0, nullptr, 0, nullptr, false))
#else
		if (+vulkan.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
#endif
		{
			Renderer renderer(win, vulkan);
			while (+win.ProcessWindowEvents())
			{
				if (+vulkan.StartFrame(2, clrAndDepth))
				{
					// TODO: Part 4b
					renderer.UpdateCamera();
					renderer.Render();
					vulkan.EndFrame(true);
				}
			}
		}
	}
	return 0; // that's all folks
}
