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
#include <fstream>

// open some namespaces to compact the code a bit
using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;

GW::MATH::GVECTORF ReadVectorFromString(std::string& str)
{
	GW::MATH::GVECTORF V = {};
	unsigned int VIndex = 0;
	std::string Num = "";

	for (unsigned int i = 0; i < str.length(); ++i)
	{
		unsigned int AcsiiChar = static_cast<unsigned int>(str[i]);
		if (str[i] == ',')
		{
			V.data[VIndex++] = std::atof(Num.c_str());
			Num.clear();
		}
		else if ((AcsiiChar >= 48 && AcsiiChar <= 57) || AcsiiChar == 46 || AcsiiChar == 45)
		{
			Num.push_back(str[i]);
		}
	}

	return V;
}

std::vector<GW::MATH::GMATRIXF>& ReadGameLevelFile()
{
	std::vector<GW::MATH::GMATRIXF> Matrices;

	std::ifstream FileHandle("../GameLevel.txt");
	std::string Line = "";
	GW::MATH::GMATRIXF Matrix;

	std::cout << "Opening File: GameLevel.txt\n";
	if (FileHandle.is_open())
	{
		std::cout << "File Opened: GameLevel.txt\n";
		while (true)
		{
			std::getline(FileHandle, Line);

			if (FileHandle.eof()) { break; }

			if (std::strcmp(Line.c_str(), "MESH") == 0)
			{
				/* Read Mesh Infomation */
				std::cout << "[Mesh Found]: ";

				std::getline(FileHandle, Line, '\n');

				std::cout << Line << "\n";

				/* Read the Matrix */

				// Read Matrix Header
				std::getline(FileHandle, Line, '(');
				std::cout << Line << "("; // Print Matrix Header

				std::getline(FileHandle, Line, ')'); // read row 1
				std::cout << Line << ")";
				Matrix.row1 = ReadVectorFromString(Line);

				std::getline(FileHandle, Line, ')'); // read row 2
				std::cout << Line << ")";
				Matrix.row2 = ReadVectorFromString(Line);

				std::getline(FileHandle, Line, ')'); // read row 3
				std::cout << Line << ")";
				Matrix.row3 = ReadVectorFromString(Line);

				std::getline(FileHandle, Line, ')'); // read row 4
				std::cout << Line << ")>\n\n";
				Matrix.row4 = ReadVectorFromString(Line);

				Matrices.push_back(Matrix);
			}
		}

		FileHandle.close();
	}

	return Matrices;
}

// lets pop a window and use Vulkan to clear to a red screen
int main()
{
	GWindow win;
	GEventResponder msgs;
	GVulkanSurface vulkan;

	ReadGameLevelFile();

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
		if (+vulkan.Create(	win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT, 
							sizeof(debugLayers)/sizeof(debugLayers[0]),
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
