#include "GatewareDefine.h"
#include "renderer.h"
// open some namespaces to compact the code a bit
using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

// lets pop a window and use Vulkan to clear to a red screen
int main()
{
	_CrtDumpMemoryLeaks();
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	GWindow win;
	GEventResponder msgs;
	GVulkanSurface vulkan;

	if (+win.Create(0, 0, 800, 600, GWindowStyle::WINDOWEDBORDERED))
	{
		// TODO: Part 1a
		win.SetWindowName("Vrij Patel + Level Renderer Project - Vulkan");
		VkClearValue clrAndDepth[2];
		clrAndDepth[0].color = { {0.25f, 0.25f, 0.25f, 1} }; // TODO: Part 1a
		clrAndDepth[1].depthStencil = { 1.0f, 0u };
		msgs.Create([&](const GW::GEvent& e) {
			GW::SYSTEM::GWindow::Events q;
			if (+e.Read(q) && q == GWindow::Events::RESIZE)
				//clrAndDepth[0].color.float32[2] += 0.01f; // disable
				std::cout << "\n[GWindow]: resized";
			});
		win.Register(msgs);
		const char* DeviceExtensions[2] =
		{
			"VK_EXT_descriptor_indexing",
			"VK_KHR_multiview"
		};

#ifndef NDEBUG
		const char* debugLayers[] = {
			//"VK_LAYER_KHRONOS_validation", // standard validation layer
			"VK_LAYER_RENDERDOC_Capture" // add this if you have installed RenderDoc
			//"VK_LAYER_LUNARG_standard_validation", // add if not on MacOS
		};		

		if (+vulkan.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT,
			sizeof(debugLayers) / sizeof(debugLayers[0]),
			debugLayers, 0, nullptr, 2, DeviceExtensions, false))
#else
		if (+vulkan.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT, 0, nullptr, 0, nullptr, 2, DeviceExtensions, false))
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
