// minimalistic code to draw a single triangle, this is not part of the API.
#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#ifdef _WIN32 // must use MT platform DLL libraries on windows
#pragma comment(lib, "shaderc_combined.lib") 
#endif

#include "Math/Matrix4D.h"
#include "FileHelper.h"
#include "Level.h"
#include "StaticMesh.h"
#include "Frustum.h"
#include "VulkanPipeline.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_vulkan.h"

#define ENABLE_FRUSTUM_CULLING 1
#define DRAW_LIGHTS 0

// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

struct ConstantBuffer
{
	uint32 MeshID; // 4 bytes
	uint32 MaterialID;// 4 bytes
	uint32 DiffuseTextureID;// 4 bytes
	uint32 ViewMatID;// 4 bytes

	Vector3D Color;// 12 bytes
	uint32 SpecularTextureID; // 4 bytes

	Vector3D FresnelColor;// 12 bytes
	uint32 NormalTextureID; // 4 bytes

	uint32 Padding[20];
};

#define GREEN Vector3D(0,1,0)
#define RED Vector3D(1,0,0)
#define BLUE Vector3D(0,0,1)

struct StagingBuffer
{
	VkDevice Device;
	VkBuffer Buffer = VK_NULL_HANDLE;
	VkDeviceMemory Memory = VK_NULL_HANDLE;

	VkDescriptorBufferInfo Descriptor;
	VkDeviceSize Size = 0;
	VkDeviceSize Alignment = 0;

	VkBufferUsageFlags UsageFlags;

	VkMemoryPropertyFlags MemoryPropertyFlags;

	void* Mapped = nullptr;

	VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
	{
		return vkMapMemory(Device, Memory, offset, size, 0, &Mapped);
	}

	void Unmap()
	{
		if (Mapped)
		{
			vkUnmapMemory(Device, Memory);
			Mapped = nullptr;
		}
	}

	void SetupDescriptor(VkDeviceSize = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
	{
		Descriptor.offset = offset;
		Descriptor.buffer = Buffer;
		Descriptor.range = Size;
	}

	VkResult Bind(VkDeviceSize offset = 0)
	{
		return vkBindBufferMemory(Device, Buffer, Memory, offset);
	}

	void Destroy()
	{
		if (Buffer)
		{
			vkDestroyBuffer(Device, Buffer, nullptr);
		}
		if (Memory)
		{
			vkFreeMemory(Device, Memory, nullptr);
		}
	}
};


// UI params are set via push constants
struct PushConstBlock {
	Vector2D scale;
	Vector2D translate;
} pushConstBlock;

// Creation, Rendering & Cleanup
class Renderer
{
	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GVulkanSurface vlk;
	GW::CORE::GEventReceiver shutdown;
	GW::INPUT::GInput Input;
	GW::INPUT::GController Controller;

	float TimePassed;
	GW::MATH::GMatrix Matrix;
	//GW::MATH::GMATRIXF ViewProjectionMatrix;
	// what we need at a minimum to draw a triangle
	VkDevice device = nullptr;
	//VkBuffer vertexHandle = nullptr;
	//VkDeviceMemory vertexData = nullptr;
	VkShaderModule VertexShader_Normal = nullptr;
	VkShaderModule VertexShader_Basic = nullptr;
	VkShaderModule VertexShader_Debug = nullptr;

	VkShaderModule PixelShader_Normal = nullptr;
	VkShaderModule PixelShader_Toon = nullptr;
	VkShaderModule PixelShader_Fresnel = nullptr;
	VkShaderModule PixelShader_FresnelNormal = nullptr;
	VkShaderModule PixelShader_Basic = nullptr;
	VkShaderModule PixelShader_Debug = nullptr;

	// pipeline settings for drawing (also required)
	VkPipeline Pipeline_Normal = nullptr;
	VkPipeline Pipeline_Toon = nullptr;
	VkPipeline Pipeline_Fresnel = nullptr;
	VkPipeline Pipeline_FresnelNormal = nullptr;
	VkPipeline Pipeline_Debug = nullptr;
	VkPipeline Pipeline_Debug2 = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;

	VkPipeline* CurrentPipeline = nullptr;

	Vector3D GridColor;

	VulkanPipeline PipelineCreator;

	bool CaptureInput = true;
	float CameraSpeed = 5.0f;

	Frustum CameraFrustum;

	Vector3D LightPos;

	/* ImGui*/
	VkImage fontImage;
	VkImageView fontView;
	VkDeviceMemory fontMemory;

	VkSampler ImguiSampler;

	VkDescriptorPool ImguiDescpool;
	VkDescriptorSetLayout ImguiLayout;
	VkDescriptorSet ImguiDescSet;

	VkPipelineCache ImguiPipelineCache;

	VkPipelineLayout ImguiPipelineLayout;

	VkShaderModule VertexShader_ImGui = nullptr;
	VkShaderModule PixelShader_ImGui = nullptr;

	VkPipeline ImguiPipeline;

	/* Imgui Vertex/Index Buffers */
	uint32 ImguiVertexCount = 0;
	uint32 ImguiIndexCount = 0;
	VkBuffer ImguiVertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory ImguiVertexBufferData = nullptr;

	VkBuffer ImguiIndexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory ImguiIndexBufferData = nullptr;

	void* ImguiVertexBufferMapped = nullptr;
	void* ImguiIndexBufferMapped = nullptr;

public:

	Level* World;
	/* Collection of all static meshes */
	std::vector<StaticMesh> StaticMeshes;
	std::vector<StaticMesh> RenderMeshes;

	std::vector<StaticMesh> DebugMeshes;

	bool SceneHasTextures = false;

	GW::MATH::GMATRIXF CameraView1;
	GW::MATH::GMATRIXF CameraView2;
	GW::MATH::GMATRIXF CameraView3;

	bool isCamView1MainCamera = true;
	bool isCamView2MainCamera = true;

	bool UseNormalShader = false;

	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk)
	{
		using namespace GW::MATH;

		TimePassed = 0.0f;

		win = _win;
		vlk = _vlk;
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		// TODO: Part 4a
		Input.Create(_win);
		Controller.Create();

		// TODO: Part 2a -> Create world matrix 
		Matrix.Create(); // Create/enable proxy 

		/* ImGui */


		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();

		// Color scheme
		ImGuiStyle& style = ImGui::GetStyle();
		style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
		// Dimensions
		io.DisplaySize = ImVec2(width, height);
		io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

		/***************** GEOMETRY INTIALIZATION ******************/
		// Grab the device & physical device so we can allocate some stuff
		VkPhysicalDevice physicalDevice = nullptr;
		vlk.GetDevice((void**)&device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);

		World = new Level(&device, &vlk, &pipelineLayout, "Vrixic", "../Levels/NormalMapTest.txt");

		/***************** SHADER INTIALIZATION ******************/
		// Intialize runtime shader compiler HLSL -> SPIRV
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
		// TODO: Part 3C
		shaderc_compile_options_set_invert_y(options, true);
#ifndef NDEBUG
		shaderc_compile_options_set_generate_debug_info(options);
#endif
		std::string VertexShaderBasicSource = FileHelper::LoadShaderFileIntoString("../Shaders/BasicVertex.hlsl");
		std::string PixelShaderBasicSource = FileHelper::LoadShaderFileIntoString("../Shaders/BasicPixel.hlsl");

		// Create Vertex Shader
		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
			compiler, VertexShaderBasicSource.c_str(), VertexShaderBasicSource.length(),
			shaderc_vertex_shader, "main.vert", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &VertexShader_Basic);
		shaderc_result_release(result); // done
		// Create Pixel Shader
		result = shaderc_compile_into_spv( // compile
			compiler, PixelShaderBasicSource.c_str(), PixelShaderBasicSource.length(),
			shaderc_fragment_shader, "main.frag", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &PixelShader_Basic);
		shaderc_result_release(result); // done

		std::string VertexShaderDebugSource = FileHelper::LoadShaderFileIntoString("../Shaders/DebugVertex.hlsl");
		std::string PixelShaderDebugSource = FileHelper::LoadShaderFileIntoString("../Shaders/DebugPixel.hlsl");

		// Create Vertex Shader
		result = shaderc_compile_into_spv( // compile
			compiler, VertexShaderDebugSource.c_str(), VertexShaderDebugSource.length(),
			shaderc_vertex_shader, "main.vert", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &VertexShader_Debug);
		shaderc_result_release(result); // done
		// Create Pixel Shader
		result = shaderc_compile_into_spv( // compile
			compiler, PixelShaderDebugSource.c_str(), PixelShaderDebugSource.length(),
			shaderc_fragment_shader, "main.frag", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &PixelShader_Debug);
		shaderc_result_release(result); // done

		std::string VertexShaderNormalSource = FileHelper::LoadShaderFileIntoString("../Shaders/NormalVertex.hlsl");
		std::string PixelShaderNormalSource = FileHelper::LoadShaderFileIntoString("../Shaders/NormalPixel.hlsl");

		// Create Vertex Shader
		result = shaderc_compile_into_spv( // compile
			compiler, VertexShaderNormalSource.c_str(), VertexShaderNormalSource.length(),
			shaderc_vertex_shader, "main.vert", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &VertexShader_Normal);
		shaderc_result_release(result); // done
		// Create Pixel Shader
		result = shaderc_compile_into_spv( // compile
			compiler, PixelShaderNormalSource.c_str(), PixelShaderNormalSource.length(),
			shaderc_fragment_shader, "main.frag", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &PixelShader_Normal);
		shaderc_result_release(result); // done

		std::string PixelShaderToonSource = FileHelper::LoadShaderFileIntoString("../Shaders/ToonPixel.hlsl");

		result = shaderc_compile_into_spv( // compile
			compiler, PixelShaderToonSource.c_str(), PixelShaderToonSource.length(),
			shaderc_fragment_shader, "main.frag", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &PixelShader_Toon);
		shaderc_result_release(result); // done

		std::string PixelShaderFresnelSource = FileHelper::LoadShaderFileIntoString("../Shaders/FresnelPixel.hlsl");

		result = shaderc_compile_into_spv( // compile
			compiler, PixelShaderFresnelSource.c_str(), PixelShaderFresnelSource.length(),
			shaderc_fragment_shader, "main.frag", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &PixelShader_Fresnel);
		shaderc_result_release(result); // done

		std::string PixelShaderFresnelNormalSource = FileHelper::LoadShaderFileIntoString("../Shaders/FresnelNormalPixel.hlsl");

		result = shaderc_compile_into_spv( // compile
			compiler, PixelShaderFresnelNormalSource.c_str(), PixelShaderFresnelNormalSource.length(),
			shaderc_fragment_shader, "main.frag", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &PixelShader_FresnelNormal);
		shaderc_result_release(result); // done

		// Free runtime shader compiler resources
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);

		/***************** PIPELINE INTIALIZATION ******************/
		// Create Pipeline & Layout (Thanks Tiny!)
		VkRenderPass renderPass;
		vlk.GetRenderPass((void**)&renderPass);

		// Create Stage Info for Vertex Shader
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, &VertexShader_Normal);
		// Create Stage Info for Fragment Shader
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, &PixelShader_Normal);

		// Assembly State
		PipelineCreator.SetInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		// Vertex Input State
		PipelineCreator.AddNewVertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
		// Vertex attribs descs
		PipelineCreator.AddNewVertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, 0);
		PipelineCreator.AddNewVertexInputAttributeDescription(1, 0, VK_FORMAT_R32G32B32_SFLOAT, 8);
		PipelineCreator.AddNewVertexInputAttributeDescription(2, 0, VK_FORMAT_R32G32B32_SFLOAT, 20);
		PipelineCreator.AddNewVertexInputAttributeDescription(3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 32);

		// Set Vertex input state create info 
		PipelineCreator.SetVertexInputStateCreateInfo();

		// Viewport State (we still need to set this up even though we will overwrite the values)
		Vector2D Pos = Vector2D(0.0f, 0.0f);
		Vector2D WidthHeight = Vector2D(static_cast<float>(width), static_cast<float>(height));
		Vector2D MinMaxDepth = Vector2D(0.0f, 1.0f);
		PipelineCreator.AddNewViewport(Pos, WidthHeight, MinMaxDepth);
		PipelineCreator.AddNewScissor(0, 0, width, height);

		// Create the viewport state create info
		PipelineCreator.SetViewportStateCreateInfo();

		// Rasterizer State
		PipelineCreator.SetDefaultRasterizationStateCreateInfo();

		// Multisampling State
		PipelineCreator.SetDefaultMultisampleStateCreateInfo();

		// Depth-Stencil State
		PipelineCreator.SetDefaultDepthStencilStateCreateInfo();

		// Color Blending Attachment & State
		PipelineCreator.SetDefaultColorBlendAttachmentState();

		PipelineCreator.SetDefaultColorBlendStateCreateInfo();

		// Dynamic State 
		// By setting these we do not need to re-create the pipeline on Resize
		PipelineCreator.AddNewDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
		PipelineCreator.AddNewDynamicState(VK_DYNAMIC_STATE_SCISSOR);
		PipelineCreator.SetDynamicStateCreateInfo();

		/* Load World Before pipeline creation */
		Frustum CamF;
		float AspectRatio = 0.0f;
		vlk.GetAspectRatio(AspectRatio);

		/* Load the file */
		std::vector<RawMeshData> RawData;
		FileHelper::ReadGameLevelFile("../Levels/NormalMapTest.txt", RawData);

		H2B::VERTEX V;
		V.pos = reinterpret_cast<H2B::VECTOR&>(CamF.FarPlaneTopLeft);

		RawMeshData Data;
		Data.IndexCount = Frustum::GetFrustumIndexCount();
		Data.VertexCount = Frustum::GetFrustumVertexCount();

		Data.Vertices.resize(Frustum::GetFrustumVertexCount());
		for (uint32 i = 0; i < Frustum::GetFrustumVertexCount(); ++i)
		{
			Data.Vertices[i] = V; // empty vector
		}

		Data.Indices.resize(Frustum::GetFrustumIndexCount());
		uint32* Indices = Frustum::GetFrustumIndices();
		for (uint32 i = 0; i < Frustum::GetFrustumIndexCount(); ++i)
		{
			Data.Indices[i] = Indices[i];
		}

		Data.InstanceCount = 1;
		Data.MaterialCount = 1;
		Data.MeshCount = 1;

		RawMaterial Mat = { };
		Data.Materials.push_back(Mat); // random mat

		Data.WorldMatrices.push_back(Matrix4D::Identity());

		H2B::MESH M;
		M.materialIndex = 0;
		M.name = "nullptr";
		M.drawInfo.indexCount = Frustum::GetFrustumIndexCount();
		M.drawInfo.indexOffset = 0;

		Data.Meshes.push_back(M);

		RawData.push_back(Data);

		StaticMeshes = World->Load(RawData);

		StaticMesh MeshCpy = StaticMeshes[StaticMeshes.size() - 1];
		DebugMeshes.push_back(MeshCpy);
		StaticMeshes.pop_back();

		// Descriptor pipeline layout

		// Before we set the layout we need to specify push constants if any
		PipelineCreator.AddNewPushConstantRange(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			sizeof(ConstantBuffer));

		std::vector<VkDescriptorSetLayout*> DescSetLayouts = World->GetShaderStorageDescSetLayouts();
		PipelineCreator.SetLayoutCreateInfo(DescSetLayouts.size(), *DescSetLayouts.data());
		PipelineCreator.CreatePipelineLayout(&device, pipelineLayout);

		// Pipeline State... (FINALLY) 
		PipelineCreator.SetGraphicsPipelineCreateInfo(&pipelineLayout, &renderPass);
		PipelineCreator.CreateGraphicsPipelines(&device, Pipeline_Normal);

		PipelineCreator.ClearStageCreateInfos();
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, &VertexShader_Normal);
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, &PixelShader_Toon);
		PipelineCreator.CreateGraphicsPipelines(&device, Pipeline_Toon);

		PipelineCreator.ClearStageCreateInfos();
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, &VertexShader_Normal);
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, &PixelShader_Fresnel);
		PipelineCreator.CreateGraphicsPipelines(&device, Pipeline_Fresnel);

		PipelineCreator.ClearStageCreateInfos();
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, &VertexShader_Normal);
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, &PixelShader_FresnelNormal);
		PipelineCreator.CreateGraphicsPipelines(&device, Pipeline_FresnelNormal);


		/* Reset the layout create info expect this time we dont want textures
		* TODO: Not make this hardcoded size to one
		*/
		PipelineCreator.SetInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
		PipelineCreator.ClearStageCreateInfos();
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, &VertexShader_Basic);
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, &PixelShader_Basic);
		PipelineCreator.CreateGraphicsPipelines(&device, Pipeline_Debug);

		PipelineCreator.ClearStageCreateInfos();
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, &VertexShader_Debug);
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, &PixelShader_Debug);
		PipelineCreator.CreateGraphicsPipelines(&device, Pipeline_Debug2);

		CurrentPipeline = &Pipeline_Normal;

		CameraView1 = World->GetViewMatrix1();
		CameraView2 = World->GetViewMatrix2();
		CameraView3 = World->GetViewMatrix3();

		GridColor = Vector3D::ZeroVector();


#if DRAW_LIGHTS
		LightPos = Vector3D(World->ShaderSceneData[0].PointLights[0].Position.X, World->ShaderSceneData[0].PointLights[0].Position.Y, World->ShaderSceneData[0].PointLights[0].Position.Z);
#endif

		InitImguiResources();

		/***************** CLEANUP / SHUTDOWN ******************/
		// GVulkanSurface will inform us when to release any allocated resources
		shutdown.Create(vlk, [&]() {
			if (+shutdown.Find(GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES, true)) {
				CleanUp(); // unlike D3D we must be careful about destroy timing
			}
			});
	}

	void RenderImGui(VkCommandBuffer& commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ImguiPipeline);

		/* ImGui */
		NewImguiFrame();
		updateBuffers();
		drawFrame(commandBuffer);
		/* ImGui */
	}

	void Render()
	{
		// grab the current Vulkan commandBuffer
		unsigned int currentBuffer;
		vlk.GetSwapchainCurrentImage(currentBuffer);
		VkCommandBuffer commandBuffer;
		vlk.GetCommandBuffer(currentBuffer, (void**)&commandBuffer);
		// what is the current client area dimensions?
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);

		// setup the pipeline's dynamic settings
		VkViewport viewport = {
			0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
		};
		VkRect2D scissor = { {0, 0}, {width, height} };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		World->Bind();

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *CurrentPipeline);
		ConstantBuffer Buffer = { };

		/* Calc Color */
		GridColor += Vector3D(Math::RandomRange(1.0f, 5.0f), Math::RandomRange(1.0f, 10.0f), Math::RandomRange(1.0f, 20.0f)) * TimePassed * 0.01f;

		Vector3D TempColor = Vector3D::ZeroVector();
		TempColor.X = (sin(GridColor.X * 0.1f) + 1.0f) * 0.5f;
		TempColor.Y = (sin(GridColor.Y * 0.1f) + 1.0f) * 0.5f;
		TempColor.Z = (sin(GridColor.Z * 0.1f) + 1.0f) * 0.5f;

		Buffer.FresnelColor = TempColor;

		for (uint32 i = 0; i < RenderMeshes.size(); ++i)
		{
			Buffer.MeshID = RenderMeshes[i].GetWorldMatrixIndex();

			for (uint32 j = 0; j < RenderMeshes[i].GetMeshCount(); ++j)
			{
				Buffer.MaterialID = RenderMeshes[i].GetMaterialIndex() + RenderMeshes[i].GetSubMeshMaterialIndex(j);
				Buffer.DiffuseTextureID = RenderMeshes[i].GetSubMeshDiffuseTextureIndex(j);
				Buffer.SpecularTextureID = RenderMeshes[i].GetSubMeshSpecularTextureIndex(j);
				Buffer.NormalTextureID = RenderMeshes[i].GetSubMeshNormalTextureIndex(j);
				Buffer.ViewMatID = 0;

				vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT |
					VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ConstantBuffer), &Buffer);
				vkCmdDrawIndexed(commandBuffer,
					RenderMeshes[i].GetSubMeshIndexCount(j),
					RenderMeshes[i].GetInstanceCount(),
					RenderMeshes[i].GetIndexOffset() + RenderMeshes[i].GetSubMeshIndexOffset(j),
					RenderMeshes[i].GetVertexOffset(), 0);
			}
		}



#if DRAW_LIGHTS
		/* Draw AABBS */
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline_Debug);


		Matrix4D Mat = Matrix4D::Identity();
		//Mat.ScaleMatrix(-0.75f);
		World->ShaderSceneData[0].PointLights[0].Position = LightPos;
		Mat.SetTranslation(LightPos);
		World->ShaderSceneData[0].WorldMatrices[0] = Mat;

		Buffer.MeshID = 0;// StaticMeshes[i].GetWorldMatrixIndex();
		Buffer.ViewMatID = 0;
		Buffer.Color = StaticMeshes[0].Color_AABB;

		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT |
			VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ConstantBuffer), &Buffer);
		vkCmdDrawIndexed(commandBuffer,
			24,
			StaticMeshes[0].GetInstanceCount(),
			World->GetLevelData()->DebugBoxIndexStart,
			World->GetLevelData()->DebugBoxVertexStart, 0);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *CurrentPipeline);
#endif // DRAW_LIGHTS


		/*--------------------------------------------------DEBUG-------------------------------------------------------*/
		//{
		//	viewport.x = 50;
		//	viewport.y = 50;
		//	viewport.width = 200;
		//	viewport.height = 200;
		//	scissor.offset.x = 50;
		//	scissor.offset.y = 50;
		//	scissor.extent.width = 200;
		//	scissor.extent.height = 200;

		//	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		//	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		//	for (uint32 i = 0; i < RenderMeshes.size(); ++i)
		//	{
		//		Buffer.MeshID = RenderMeshes[i].GetWorldMatrixIndex();

		//		for (uint32 j = 0; j < RenderMeshes[i].GetMeshCount(); ++j)
		//		{
		//			Buffer.MaterialID = RenderMeshes[i].GetMaterialIndex() + RenderMeshes[i].GetSubMeshMaterialIndex(j);
		//			Buffer.DiffuseTextureID = RenderMeshes[i].GetSubMeshDiffuseTextureIndex(j);
		//			Buffer.SpecularTextureID = RenderMeshes[i].GetSubMeshSpecularTextureIndex(j);
		//			Buffer.NormalTextureID = RenderMeshes[i].GetSubMeshNormalTextureIndex(j);
		//			Buffer.ViewMatID = 1;

		//			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT |
		//				VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ConstantBuffer), &Buffer);
		//			vkCmdDrawIndexed(commandBuffer,
		//				RenderMeshes[i].GetSubMeshIndexCount(j),
		//				RenderMeshes[i].GetInstanceCount(),
		//				RenderMeshes[i].GetIndexOffset() + RenderMeshes[i].GetSubMeshIndexOffset(j),
		//				RenderMeshes[i].GetVertexOffset(), 0);
		//		}
		//	}


		//	/* Draw AABBS */
		//	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline_Debug);

		//	for (uint32 i = 0; i < StaticMeshes.size(); ++i)
		//	{
		//		Buffer.MeshID = StaticMeshes[i].GetWorldMatrixIndex();
		//		Buffer.ViewMatID = 1;
		//		Buffer.Color = StaticMeshes[i].Color_AABB;

		//		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT |
		//			VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ConstantBuffer), &Buffer);
		//		vkCmdDrawIndexed(commandBuffer,
		//			24,
		//			StaticMeshes[i].GetInstanceCount(),
		//			World->GetLevelData()->DebugBoxIndexStart,
		//			World->GetLevelData()->DebugBoxVertexStart + (i * 8), 0);
		//	}

		//	/* Draw Frustum */
		//	Buffer.MeshID = DebugMeshes[0].GetWorldMatrixIndex();
		//	Buffer.ViewMatID = 1;
		//	Buffer.Color = Vector3D::ZeroVector();

		//	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT |
		//		VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ConstantBuffer), &Buffer);
		//	vkCmdDrawIndexed(commandBuffer,
		//		Frustum::GetFrustumIndexCount(),
		//		1,
		//		World->GetLevelData()->DebugBoxIndexStart - 24,
		//		World->GetLevelData()->DebugBoxVertexStart - 8, 0);
		//}

		{
			viewport.x = width - 210;// 640;
			viewport.y = 50;
			viewport.width = 200;
			viewport.height = 200;
			scissor.offset.x = width - 210;// 640;
			scissor.offset.y = 50;
			scissor.extent.width = 200;
			scissor.extent.height = 200;

			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			/* Draw AABBS */
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline_Debug);

			for (uint32 i = 0; i < StaticMeshes.size(); ++i)
			{
				Buffer.MeshID = StaticMeshes[i].GetWorldMatrixIndex();
				Buffer.ViewMatID = 2;
				Buffer.Color = StaticMeshes[i].Color_AABB;

				vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT |
					VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ConstantBuffer), &Buffer);
				vkCmdDrawIndexed(commandBuffer,
					24,
					StaticMeshes[i].GetInstanceCount(),
					World->GetLevelData()->DebugBoxIndexStart,
					World->GetLevelData()->DebugBoxVertexStart + (i * 8), 0);
			}

			/* Draw Frustum */
			Buffer.MeshID = DebugMeshes[0].GetWorldMatrixIndex();
			Buffer.ViewMatID = 2;
			Buffer.Color = Vector3D::ZeroVector();

			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT |
				VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ConstantBuffer), &Buffer);
			vkCmdDrawIndexed(commandBuffer,
				Frustum::GetFrustumIndexCount(),
				1,
				World->GetLevelData()->DebugBoxIndexStart - 24,
				World->GetLevelData()->DebugBoxVertexStart - 8, 0);

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline_Debug2);

			std::vector<Vertex>* Vertices = World->GetLevelData()->GetVertices();
			for (uint32 i = Vertices->size() - 12; i < Vertices->size(); i += 2)
			{
				Buffer.Color = Vector3D((*Vertices)[i].Color.X, (*Vertices)[i].Color.Y, (*Vertices)[i].Color.Z);
				vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT |
					VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ConstantBuffer), &Buffer);
				vkCmdDraw(commandBuffer, 2, 1, i, 0);
			}
		}
		/*--------------------------------------------------DEBUG-------------------------------------------------------*/

		RenderImGui(commandBuffer);
	}
	// TODO: Part 4b
	void UpdateCamera()
	{
		using namespace GW::MATH;
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

		// Define All Of the Key/Controller State Variables 
		float RightTriggerState = 0, LeftTriggerState = 0, LeftStickYAxis = 0, LeftStickXAxis = 0, RightStickYAxis = 0, RightStickXAxis = 0;
		float WKeyState, SKeyState = 0, AKeyState = 0, DKeyState = 0, SpaceKeyState = 0, LeftShiftKeyState = 0;
		float MouseDeltaX = 0, MouseDeltaY = 0;
		unsigned int ScreenHeight = 0, ScreenWidth = 0;
		float FKeyState = 0;
		float LKeyState = 0;
		float OKeyState = 0;
		float PKeyState = 0;
		float UKeyState = 0;
		float OneKeyState = 0;
		float TwoKeyState = 0;
		float ThreeKeyState = 0;
		float ZeroKeyState = 0;
		float NineKeyState = 0;
		float EightKeyState = 0;
		float SevenKeyState = 0;
		float MouseRightState = 0;
		float UpKeyState = 0;
		float DownKeyState = 0;
		float LeftKeyState = 0;
		float RightKeyState = 0;
		float CameraRotationSpeed = 50.0f;

		float MouseScrollUp = 0, MouseScrollDown = 0;

		// Get Screen Width/Height
		win.GetWidth(ScreenWidth);
		win.GetHeight(ScreenHeight);

		// Get The Key/Controller States
		Input.GetState(G_KEY_SPACE, SpaceKeyState);
		Input.GetState(G_KEY_LEFTSHIFT, LeftShiftKeyState);

		Input.GetState(G_KEY_W, WKeyState);
		Input.GetState(G_KEY_O, OKeyState);
		Input.GetState(G_KEY_P, PKeyState);
		Input.GetState(G_KEY_UP, UpKeyState);
		Input.GetState(G_KEY_A, AKeyState);
		Input.GetState(G_KEY_LEFT, LeftKeyState);
		Input.GetState(G_KEY_S, SKeyState);
		Input.GetState(G_KEY_DOWN, DownKeyState);
		Input.GetState(G_KEY_D, DKeyState);
		Input.GetState(G_KEY_RIGHT, RightKeyState);
		Input.GetState(G_KEY_F, FKeyState);
		Input.GetState(G_KEY_1, OneKeyState);
		Input.GetState(G_KEY_2, TwoKeyState);
		Input.GetState(G_KEY_3, ThreeKeyState);
		Input.GetState(G_KEY_9, NineKeyState);
		Input.GetState(G_KEY_8, EightKeyState);
		Input.GetState(G_KEY_7, SevenKeyState);
		Input.GetState(G_KEY_0, ZeroKeyState);
		Input.GetState(G_BUTTON_RIGHT, MouseRightState);

		if (FKeyState > 0)
		{
			std::string FilePath;
			if (FileHelper::OpenFileDialog(FilePath))
			{
				vkDeviceWaitIdle(device);
				delete World;
				StaticMeshes.clear();

				std::string LevelName;
				uint32 PeriodIndex = -1;
				uint32 LastSlashIndex = -1;

				for (uint32 i = FilePath.length(); i > 0; --i)
				{
					if (FilePath[i] == '\\')
					{
						LastSlashIndex = i;
						break;
					}
					else if (FilePath[i] == '.' && PeriodIndex == -1)
					{
						PeriodIndex = i;
					}
				}

				for (uint32 i = LastSlashIndex + 1; i < PeriodIndex; ++i)
				{
					LevelName.push_back(FilePath[i]);
				}

				World = new Level(&device, &vlk, &pipelineLayout, LevelName.c_str(), FilePath.c_str());

				{
					/* Load the file */
					std::vector<RawMeshData> RawData;
					FileHelper::ReadGameLevelFile(FilePath.c_str(), RawData);

					H2B::VERTEX V;
					V.pos = { 0,0,0 };

					RawMeshData Data;
					Data.IndexCount = Frustum::GetFrustumIndexCount();
					Data.VertexCount = Frustum::GetFrustumVertexCount();

					Data.Vertices.resize(Frustum::GetFrustumVertexCount());
					for (uint32 i = 0; i < Frustum::GetFrustumVertexCount(); ++i)
					{
						Data.Vertices[i] = V; // empty vector
					}

					Data.Indices.resize(Frustum::GetFrustumIndexCount());
					uint32* Indices = Frustum::GetFrustumIndices();
					for (uint32 i = 0; i < Frustum::GetFrustumIndexCount(); ++i)
					{
						Data.Indices[i] = Indices[i];
					}

					Data.InstanceCount = 1;
					Data.MaterialCount = 1;
					Data.MeshCount = 1;

					RawMaterial Mat;
					Data.Materials.push_back(Mat); // random mat

					Data.WorldMatrices.push_back(Matrix4D::Identity());

					H2B::MESH M;
					M.materialIndex = 0;
					M.name = "nullptr";
					M.drawInfo.indexCount = Frustum::GetFrustumIndexCount();
					M.drawInfo.indexOffset = 0;

					Data.Meshes.push_back(M);

					RawData.push_back(Data);

					StaticMeshes = World->Load(RawData);

					StaticMesh MeshCpy = StaticMeshes[StaticMeshes.size() - 1];
					DebugMeshes.clear();
					DebugMeshes.push_back(MeshCpy);
					StaticMeshes.pop_back();
				}

				{
					vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
					vkDestroyPipeline(device, Pipeline_Normal, nullptr);
					vkDestroyPipeline(device, Pipeline_Fresnel, nullptr);
					vkDestroyPipeline(device, Pipeline_FresnelNormal, nullptr);
					vkDestroyPipeline(device, Pipeline_Toon, nullptr);

					VkRenderPass renderPass;
					vlk.GetRenderPass((void**)&renderPass);

					std::vector<VkDescriptorSetLayout*> DescSetLayouts = World->GetShaderStorageDescSetLayouts();
					PipelineCreator.SetLayoutCreateInfo(DescSetLayouts.size(), *DescSetLayouts.data());

					PipelineCreator.CreatePipelineLayout(&device, pipelineLayout);
					PipelineCreator.SetGraphicsPipelineCreateInfo(&pipelineLayout, &renderPass);

					PipelineCreator.SetInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

					PipelineCreator.ClearStageCreateInfos();
					PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, &VertexShader_Normal);
					PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, &PixelShader_Normal);
					PipelineCreator.CreateGraphicsPipelines(&device, Pipeline_Normal);

					PipelineCreator.ClearStageCreateInfos();
					PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, &VertexShader_Normal);
					PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, &PixelShader_Toon);
					PipelineCreator.CreateGraphicsPipelines(&device, Pipeline_Toon);

					PipelineCreator.ClearStageCreateInfos();
					PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, &VertexShader_Normal);
					PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, &PixelShader_Fresnel);
					PipelineCreator.CreateGraphicsPipelines(&device, Pipeline_Fresnel);

					PipelineCreator.ClearStageCreateInfos();
					PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, &VertexShader_Normal);
					PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, &PixelShader_FresnelNormal);
					PipelineCreator.CreateGraphicsPipelines(&device, Pipeline_FresnelNormal);

				}

				CameraView1 = World->GetViewMatrix1();
				CameraView2 = World->GetViewMatrix2();
				CameraView3 = World->GetViewMatrix3();

#if DRAW_LIGHTS
				LightPos = Vector3D(World->ShaderSceneData[0].PointLights[0].Position.X, World->ShaderSceneData[0].PointLights[0].Position.Y, World->ShaderSceneData[0].PointLights[0].Position.Z);
#endif
			}
		}

		if (Input.GetState(G_KEY_L, LKeyState) == GW::GReturn::SUCCESS && LKeyState > 0)
		{
			CaptureInput = false;
		}

		if (Input.GetState(G_KEY_U, UKeyState) == GW::GReturn::SUCCESS && UKeyState > 0)
		{
			CaptureInput = true;
		}

		if (NineKeyState > 0)
		{
			CurrentPipeline = &Pipeline_Toon;
		}

		if (ZeroKeyState > 0)
		{
			CurrentPipeline = &Pipeline_Normal;
		}

		if (EightKeyState > 0)
		{
			CurrentPipeline = &Pipeline_Fresnel;
		}

		if (SevenKeyState > 0)
		{
			CurrentPipeline = &Pipeline_FresnelNormal;
		}

		if (!CaptureInput)
		{
			return;
		}

		if (MouseRightState > 0.0f)
		{

			if (Input.GetMouseDelta(MouseDeltaX, MouseDeltaY) != GW::GReturn::SUCCESS)
			{
				MouseDeltaX = 0;
				MouseDeltaY = 0;
			};

			{
				if (Input.GetState(G_MOUSE_SCROLL_UP, MouseScrollUp) != GW::GReturn::SUCCESS)
				{
					MouseScrollUp = 0;
				}
				if (Input.GetState(G_MOUSE_SCROLL_DOWN, MouseScrollDown) != GW::GReturn::SUCCESS)
				{
					MouseScrollDown = 0;
				}

				CameraSpeed += MouseScrollUp - MouseScrollDown;
				CameraSpeed = Math::Clamp(CameraSpeed, 100.0f, 0.2f);
			}

			Controller.GetState(0, G_RIGHT_TRIGGER_AXIS, RightTriggerState);
			Controller.GetState(0, G_LEFT_TRIGGER_AXIS, LeftTriggerState);
			Controller.GetState(0, G_LY_AXIS, LeftStickYAxis);
			Controller.GetState(0, G_LX_AXIS, LeftStickXAxis);
			Controller.GetState(0, G_RY_AXIS, RightStickYAxis);
			Controller.GetState(0, G_RX_AXIS, RightStickXAxis);


			GW::MATH::GMATRIXF* CamViewMatrix;

			if (OneKeyState > 0.0f)
			{
				isCamView1MainCamera = true;
				isCamView2MainCamera = false;
				CamViewMatrix = &CameraView1;
			}
			if (TwoKeyState > 0.0f)
			{
				isCamView1MainCamera = false;
				isCamView2MainCamera = true;
				CamViewMatrix = &CameraView2;
			}
			if (ThreeKeyState > 0.0f)
			{
				isCamView1MainCamera = false;
				isCamView2MainCamera = false;
				CamViewMatrix = &CameraView3;
			}

			if (isCamView1MainCamera)
			{
				CamViewMatrix = &CameraView1;
			}
			else if (isCamView2MainCamera)
			{
				CamViewMatrix = &CameraView2;
			}
			else
			{
				CamViewMatrix = &CameraView3;
			}

			LeftShiftKeyState = 0.0;

			// Light pos
			LightPos.X += (LeftKeyState - RightKeyState) * 0.02f;
			LightPos.Z += (DownKeyState - UpKeyState) * 0.02f;
			LightPos.Y += (OKeyState - PKeyState) * 0.02f;

			// Calculate The Deltas 		
			float DeltaX = DKeyState - AKeyState + LeftStickXAxis;
			float DeltaY = (SpaceKeyState - LeftShiftKeyState) + (RightTriggerState - LeftTriggerState);
			float DeltaZ = WKeyState - SKeyState + LeftStickYAxis;
			float ThumbSpeed = PI * TimePassed;

			float TotalPitch = (Math::DegreesToRadians(65.0f) * MouseDeltaY / ScreenHeight + RightStickYAxis * -ThumbSpeed) * CameraRotationSpeed;
			float TotalYaw = (Math::DegreesToRadians(65.0f) * MouseDeltaX / ScreenWidth + RightStickXAxis * ThumbSpeed) * CameraRotationSpeed;

			// Camera Variables
			float PerFrameSpeed = TimePassed * CameraSpeed;

			// TODO: Part 4c
			Matrix.InverseF(CameraView1, CameraView1);
			Matrix.InverseF(CameraView2, CameraView2);
			Matrix.InverseF(CameraView3, CameraView3);

			// TODO: Part 4d -> Camera Movement Y
			GMATRIXF CameraTranslationMatrix;
			Matrix.IdentityF(CameraTranslationMatrix);

			GW::MATH::GVECTORF CameraTranslateVector = { 0.0f, 0.0f, 0.0f, 1.0f };
			CameraTranslateVector.y += DeltaY * PerFrameSpeed;
			Matrix.TranslateGlobalF(CameraTranslationMatrix, CameraTranslateVector, CameraTranslationMatrix);
			Matrix.MultiplyMatrixF(*CamViewMatrix, CameraTranslationMatrix, *CamViewMatrix);
			Matrix.IdentityF(CameraTranslationMatrix);
			CameraTranslateVector.y = 0;
			// TODO: Part 4e -> Camera Movement XZ
			CameraTranslateVector.x = DeltaX * PerFrameSpeed;
			CameraTranslateVector.z = DeltaZ * PerFrameSpeed;
			Matrix.TranslateLocalF(CameraTranslationMatrix, CameraTranslateVector, CameraTranslationMatrix);
			Matrix.MultiplyMatrixF(CameraTranslationMatrix, *CamViewMatrix, *CamViewMatrix);

			World->UpdateCameraWorldPosition(CamViewMatrix->row4);

			// TODO: Part 4f -> Pitch Rotation 
			GMATRIXF PitchMatrix;
			Matrix.IdentityF(PitchMatrix);
			Matrix.RotateXLocalF(PitchMatrix, Math::DegreesToRadians(TotalPitch), PitchMatrix);
			Matrix.MultiplyMatrixF(PitchMatrix, *CamViewMatrix, *CamViewMatrix);
			// TODO: Part 4g -> Yaw Rotation
			GMATRIXF YawMatrix;
			Matrix.IdentityF(YawMatrix);
			Matrix.RotateYGlobalF(YawMatrix, Math::DegreesToRadians(TotalYaw), YawMatrix);
			GVECTORF CameraPosition = CamViewMatrix->row4;
			Matrix.MultiplyMatrixF(*CamViewMatrix, YawMatrix, *CamViewMatrix);
			CamViewMatrix->row4 = CameraPosition;

			/*--------------------------------------------------DEBUG-------------------------------------------------------*/
			//Frustum CamF;
			float AspectRatio = 0.0f;
			vlk.GetAspectRatio(AspectRatio);
#if 1 // kept for later 
			Vector4D U = reinterpret_cast<Vector4D&>(CameraView1.row2);
			Vector4D Ri = reinterpret_cast<Vector4D&>(CameraView1.row1);
			Vector4D F = reinterpret_cast<Vector4D&>(CameraView1.row3);
			Vector4D P = reinterpret_cast<Vector4D&>(CameraView1.row4);

			Vector3D UP = Vector3D(U.X, U.Y, U.Z);
			Vector3D Right = Vector3D(Ri.X, Ri.Y, Ri.Z);
			Vector3D Pos = Vector3D(P.X, P.Y, P.Z);
			Vector3D Forw = Vector3D(F.X, F.Y, F.Z);
#endif

#if ENABLE_FRUSTUM_CULLING
			std::vector<Vertex>* Vertices = World->GetLevelData()->GetVertices();
			CameraFrustum.SetFrustumInternals(AspectRatio, 65.0f, 0.01f, 1000.0f);
			//CameraFrustum.CreateFrustum(CameraView1, 1.0f, AspectRatio, 0.01f, 100.0f);

			CameraFrustum.CreateFrustum(Pos, Right, UP, Forw);
			CameraFrustum.DebugUpdateNormalVerts(World->GetLevelData()->GetVertices()->size() - 12, World->GetLevelData()->GetVertices());
			CameraFrustum.DebugUpdateVertices(World->GetLevelData()->DebugBoxVertexStart - Frustum::GetFrustumVertexCount(), Vertices);
			World->GetLevelData()->UpdateVertexBuffer();
#endif

			if (isCamView1MainCamera)
			{
				CameraView2 = GW::MATH::GIdentityMatrixF;
				CameraView2.row4 = CameraView1.row4;
				GW::MATH::GVECTORF Vec = { 0.0, 0.0f, -250.0f,1.0f };
				Matrix.RotateYGlobalF(CameraView2, Math::DegreesToRadians(90), CameraView2);
				Matrix.TranslateLocalF(CameraView2, Vec, CameraView2);
			}

			Matrix.InverseF(CameraView1, CameraView1);
			Matrix.InverseF(CameraView2, CameraView2);
			Matrix.InverseF(CameraView3, CameraView3);

			World->SetViewMatrix1(CameraView1);
			World->SetViewMatrix2(CameraView2);
			World->SetViewMatrix3(CameraView3);

		}

#if ENABLE_FRUSTUM_CULLING
		uint32 RemovedGPUCalls = 0;
		RenderMeshes.clear();
		for (uint32 i = 0; i < StaticMeshes.size(); ++i)
		{
			/* --> Make the box in ndc space */
			Vector4D MinBox4D = World->ShaderSceneData->WorldMatrices[StaticMeshes[i].GetWorldMatrixIndex()] * Vector4D(StaticMeshes[i].MinBox_AABB, 1.0f);
			Vector4D MaxBox4D = World->ShaderSceneData->WorldMatrices[StaticMeshes[i].GetWorldMatrixIndex()] * Vector4D(StaticMeshes[i].MaxBox_AABB, 1.0f);

			Vector3D MinBox = Vector3D(MinBox4D.X, MinBox4D.Y, MinBox4D.Z);
			Vector3D MaxBox = Vector3D(MaxBox4D.X, MaxBox4D.Y, MaxBox4D.Z);

			if (CameraFrustum.TestAABB(MinBox, MaxBox) == PlaneIntersectionResult::Back)
			{
				StaticMeshes[i].Color_AABB = RED;
				RemovedGPUCalls++;
			}
			else
			{
				StaticMeshes[i].Color_AABB = GREEN;
				RenderMeshes.push_back(StaticMeshes[i]);
			}
		}
#endif

		float AspectRatio = 0.0f;
		vlk.GetAspectRatio(AspectRatio);
		GW::MATH::GMATRIXF ProjectionMatrix;
		GW::MATH::GMatrix::IdentityF(ProjectionMatrix);
		float FOV = Math::DegreesToRadians(65);
		float NearPlane = 0.01f;
		float FarPlane = 1000.0f;
		GW::MATH::GMatrix::ProjectionDirectXLHF(FOV, AspectRatio, NearPlane, FarPlane, ProjectionMatrix);
		World->SetProjectionMatrix(ProjectionMatrix);

		StaticMeshes[0].Rotate(TimePassed * 1.25f, TimePassed * 1.25f, 0.0f);
		//StaticMeshes[0].Update();

		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		using ms = std::chrono::duration<float, std::milli>;
		TimePassed = std::chrono::duration_cast<ms>(end - begin).count();
	}

	void NewImguiFrame()
	{
		ImGui::NewFrame();
		// Init imGui windows and elements
		// 
		//SRS - ShowDemoWindow() sets its own initial position and size, cannot override here
		//ImGui::ShowDemoWindow();

		//ImGui::End();

		// Render to generate draw buffers
		ImGui::Render();
	}

	// Update vertex and index buffer containing the imGui elements when required
	void updateBuffers()
	{
		ImDrawData* imDrawData = ImGui::GetDrawData();

		// Note: Alignment is done inside buffer creation
		VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
		VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

		if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
			return;
		}

		GW::GReturn Result;
		VkPhysicalDevice PhysicalDevice = nullptr;
		if ((Result = vlk.GetPhysicalDevice((void**)&PhysicalDevice)) != GW::GReturn::SUCCESS)
		{
			std::cout << "\n[Gateware] Failed to get PhysicalDevice....";
			return;
		}

		// Update buffers only if vertex or index count has been changed compared to current buffer size
		// Vertex buffer
		if ((ImguiVertexBuffer == VK_NULL_HANDLE) || (ImguiVertexCount != imDrawData->TotalVtxCount))
		{
			{
				if (ImguiVertexBufferMapped)
				{
					vkUnmapMemory(device, ImguiVertexBufferData);
					ImguiVertexBufferMapped = nullptr;
				}

				if (ImguiVertexBuffer)
				{
					vkDeviceWaitIdle(device);
					vkDestroyBuffer(device, ImguiVertexBuffer, nullptr);
					vkFreeMemory(device, ImguiVertexBufferData, nullptr);
				}
			}

			{

				GvkHelper::create_buffer(PhysicalDevice, device, vertexBufferSize,
					VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ImguiVertexBuffer, &ImguiVertexBufferData);
			}

			ImguiVertexCount = imDrawData->TotalVtxCount;
			ImguiVertexBufferMapped = new ImDrawVert[ImguiVertexCount];
			vkMapMemory(device, ImguiVertexBufferData, 0, VK_WHOLE_SIZE, 0, &ImguiVertexBufferMapped);
		}

		// Index buffer
		if ((ImguiIndexBuffer == VK_NULL_HANDLE) || (ImguiIndexCount < imDrawData->TotalIdxCount))
		{
			{
				if (ImguiIndexBufferMapped)
				{
					vkUnmapMemory(device, ImguiIndexBufferData);
					ImguiIndexBufferMapped = nullptr;
				}

				if (ImguiIndexBuffer)
				{
					vkDestroyBuffer(device, ImguiIndexBuffer, nullptr);
					vkFreeMemory(device, ImguiIndexBufferData, nullptr);
				}
			}

			GvkHelper::create_buffer(PhysicalDevice, device, indexBufferSize,
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ImguiIndexBuffer, &ImguiIndexBufferData);

			ImguiIndexCount = imDrawData->TotalIdxCount;
			ImguiIndexBufferMapped = new ImDrawIdx[ImguiIndexCount];

			vkMapMemory(device, ImguiIndexBufferData, 0, VK_WHOLE_SIZE, 0, &ImguiIndexBufferMapped);
		}

		// Upload data
		ImDrawVert* vtxDst = (ImDrawVert*)ImguiVertexBufferMapped;
		ImDrawIdx* idxDst = (ImDrawIdx*)ImguiIndexBufferMapped;

		for (int n = 0; n < imDrawData->CmdListsCount; n++) {
			const ImDrawList* cmd_list = imDrawData->CmdLists[n];
			memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtxDst += cmd_list->VtxBuffer.Size;
			idxDst += cmd_list->IdxBuffer.Size;
		}

		//GvkHelper::write_to_buffer(device, ImguiVertexBufferData, ImguiVertexBufferMapped, vertexBufferSize);
		//GvkHelper::write_to_buffer(device, ImguiIndexBufferData, ImguiIndexBufferMapped, indexBufferSize);

		// Flush to make writes visible to GPU
		VkMappedMemoryRange mappedRangeV = {};
		mappedRangeV.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRangeV.memory = ImguiVertexBufferData;
		mappedRangeV.offset = 0;
		mappedRangeV.size = VK_WHOLE_SIZE;

		vkFlushMappedMemoryRanges(device, 1, &mappedRangeV);

		VkMappedMemoryRange mappedRangeI = {};
		mappedRangeI.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRangeI.memory = ImguiIndexBufferData;
		mappedRangeI.offset = 0;
		mappedRangeI.size = VK_WHOLE_SIZE;
		vkFlushMappedMemoryRanges(device, 1, &mappedRangeI);
	}

	// Draw current imGui frame into a command buffer
	void drawFrame(VkCommandBuffer commandBuffer)
	{
		ImGuiIO& io = ImGui::GetIO();

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ImguiPipelineLayout, 0, 1, &ImguiDescSet, 0, nullptr);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ImguiPipeline);

		VkViewport viewport{};
		viewport.width = ImGui::GetIO().DisplaySize.x;
		viewport.height = ImGui::GetIO().DisplaySize.y;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		// UI scale and translate via push constants
		pushConstBlock.scale = Vector2D(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
		pushConstBlock.translate = Vector2D(-1.0f);
		vkCmdPushConstants(commandBuffer, ImguiPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

		// Render commands
		ImDrawData* imDrawData = ImGui::GetDrawData();
		int32_t vertexOffset = 0;
		int32_t indexOffset = 0;

		if (imDrawData->CmdListsCount > 0) {

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &ImguiVertexBuffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, ImguiIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

			for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
			{
				const ImDrawList* cmd_list = imDrawData->CmdLists[i];
				for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
				{
					const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
					VkRect2D scissorRect;
					scissorRect.offset.x = Math::Max((int32_t)(pcmd->ClipRect.x), 0);
					scissorRect.offset.y = Math::Max((int32_t)(pcmd->ClipRect.y), 0);
					scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
					scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
					vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
					vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
					indexOffset += pcmd->ElemCount;
				}
				vertexOffset += cmd_list->VtxBuffer.Size;
			}
		}
	}

	void InitImguiResources()
	{
		VkInstance Instance = nullptr;
		VkPhysicalDevice PhysicalDevice = nullptr;
		VkDevice Device = nullptr;
		VkQueue PresentQueue = nullptr;

		vlk.GetInstance((void**)&Instance);
		vlk.GetPhysicalDevice((void**)&PhysicalDevice);
		vlk.GetDevice((void**)&Device);
		vlk.GetPresentQueue((void**)&PresentQueue);

		ImGuiIO& io = ImGui::GetIO();

		// Create font texture
		unsigned char* fontData;
		int texWidth, texHeight;
		io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
		VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

		// Create target image for copy
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageInfo.extent.width = texWidth;
		imageInfo.extent.height = texHeight;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VK_ERROR(vkCreateImage(Device, &imageInfo, nullptr, &fontImage));
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(Device, fontImage, &memReqs);
		VkMemoryAllocateInfo memAllocInfo{};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nullptr);
		VK_ERROR(vkAllocateMemory(Device, &memAllocInfo, nullptr, &fontMemory));
		VK_ERROR(vkBindImageMemory(Device, fontImage, fontMemory, 0));

		// Image view
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = fontImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;
		VK_ERROR(vkCreateImageView(Device, &viewInfo, nullptr, &fontView));

		// Staging buffers for font data upload
		StagingBuffer stagingBuffer;

		{
			stagingBuffer.Device = Device;

			// Create the buffer handle
			VkBufferCreateInfo bufCreateInfo{};
			bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufCreateInfo.size = uploadSize;
			VK_ERROR(vkCreateBuffer(Device, &bufCreateInfo, nullptr, &stagingBuffer.Buffer));

			// Create the memory backing up the buffer handle
			VkMemoryRequirements memReqs;
			VkMemoryAllocateInfo memAlloc{};
			memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			vkGetBufferMemoryRequirements(Device, stagingBuffer.Buffer, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			// Find a memory type index that fits the properties of the buffer
			memAlloc.memoryTypeIndex = GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, nullptr);
			// If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
			VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
			if (VK_BUFFER_USAGE_TRANSFER_SRC_BIT & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
				allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
				allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
				memAlloc.pNext = &allocFlagsInfo;
			}
			VK_ERROR(vkAllocateMemory(Device, &memAlloc, nullptr, &stagingBuffer.Memory));

			stagingBuffer.Alignment = memReqs.alignment;
			stagingBuffer.Size = uploadSize;
			stagingBuffer.UsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			stagingBuffer.MemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			// If a pointer to the buffer data has been passed, map the buffer and copy over the data
			//if (data != nullptr)
			//{
			//	VK_ERROR(stagingBuffer.map());
			//	memcpy(stagingBuffer.mapped, data, size);
			//	if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			//		stagingBuffer.flush();
			//
			//	stagingBuffer.unmap();
			//}

			// Initialize a default descriptor that covers the whole buffer size
			stagingBuffer.SetupDescriptor();

			// Attach the memory to the buffer object
			VK_ERROR(stagingBuffer.Bind());
		}

		stagingBuffer.Map();
		memcpy(stagingBuffer.Mapped, fontData, uploadSize);
		stagingBuffer.Unmap();

		VkCommandPool commandPool;
		vlk.GetCommandPool((void**)&commandPool);

		// Copy buffer data to font image
		VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = commandPool; // use same command pool 
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = 1;
		VkCommandBuffer cmdBuffer;
		VK_ERROR(vkAllocateCommandBuffers(Device, &commandBufferAllocateInfo, &cmdBuffer));
		// If requested, also start recording for the new command buffer
		VkCommandBufferBeginInfo cmdBufferBeginInfo{};
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		VK_ERROR(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo));

		VkCommandBuffer copyCmd = cmdBuffer;

		// Prepare for transfer
		{
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;

			// Create an image barrier object
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageMemoryBarrier.image = fontImage;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
			switch (VK_IMAGE_LAYOUT_UNDEFINED)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				imageMemoryBarrier.srcAccessMask = 0;
				break;

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image is a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (imageMemoryBarrier.srcAccessMask == 0)
				{
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Put barrier inside setup command buffer
			vkCmdPipelineBarrier(
				copyCmd,
				VK_PIPELINE_STAGE_HOST_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		}

		// Copy
		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = texWidth;
		bufferCopyRegion.imageExtent.height = texHeight;
		bufferCopyRegion.imageExtent.depth = 1;

		vkCmdCopyBufferToImage(
			copyCmd,
			stagingBuffer.Buffer,
			fontImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&bufferCopyRegion
		);

		// Prepare for shader read
		{
			VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			VkImageLayout oldImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			VkImageLayout newImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			VkImage image = fontImage;

			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = aspectMask;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;

			// Create an image barrier object
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
			switch (oldImageLayout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				imageMemoryBarrier.srcAccessMask = 0;
				break;

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image is a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (newImageLayout)
			{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (imageMemoryBarrier.srcAccessMask == 0)
				{
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Put barrier inside setup command buffer
			vkCmdPipelineBarrier(
				copyCmd,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		}

		{
			if (copyCmd == VK_NULL_HANDLE)
			{
				return;
			}

			VK_ERROR(vkEndCommandBuffer(copyCmd));

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &copyCmd;
			// Create fence to ensure that the command buffer has finished executing
			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = 0;
			VkFence fence;
			VK_ERROR(vkCreateFence(Device, &fenceInfo, nullptr, &fence));
			// Submit to the queue
			VkQueue queue;
			vlk.GetPresentQueue((void**)&queue);
			VK_ERROR(vkQueueSubmit(queue, 1, &submitInfo, fence));
			// Wait for the fence to signal that command buffer has finished executing
			VK_ERROR(vkWaitForFences(Device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
			vkDestroyFence(Device, fence, nullptr);

			if (free)
			{
				vkFreeCommandBuffers(Device, commandPool, 1, &copyCmd);
			}
		}

		stagingBuffer.Destroy();

		// Font texture Sampler
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_ERROR(vkCreateSampler(Device, &samplerInfo, nullptr, &ImguiSampler));

		// Descriptor pool
		VkDescriptorPoolSize descriptorPoolSize{};
		descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorPoolSize.descriptorCount = 1;
		std::vector<VkDescriptorPoolSize> poolSizes = { descriptorPoolSize };

		VkDescriptorPoolCreateInfo descriptorPoolInfo{};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		descriptorPoolInfo.pPoolSizes = poolSizes.data();
		descriptorPoolInfo.maxSets = 2;

		VK_ERROR(vkCreateDescriptorPool(Device, &descriptorPoolInfo, nullptr, &ImguiDescpool));

		// Descriptor set layout
		VkDescriptorSetLayoutBinding setLayoutBinding{};
		setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBinding.binding = 0;
		setLayoutBinding.descriptorCount = 1;
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = { setLayoutBinding };

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.pBindings = setLayoutBindings.data();
		descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		VK_ERROR(vkCreateDescriptorSetLayout(Device, &descriptorSetLayoutCreateInfo, nullptr, &ImguiLayout));

		// Descriptor set
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = ImguiDescpool;
		allocInfo.pSetLayouts = &ImguiLayout;
		allocInfo.descriptorSetCount = 1;

		VK_ERROR(vkAllocateDescriptorSets(Device, &allocInfo, &ImguiDescSet));

		VkDescriptorImageInfo fontDescriptor{};
		fontDescriptor.sampler = ImguiSampler;
		fontDescriptor.imageView = fontView;
		fontDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = ImguiDescSet;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSet.dstBinding = 0;
		writeDescriptorSet.pImageInfo = &fontDescriptor;
		writeDescriptorSet.descriptorCount = 1;

		std::vector<VkWriteDescriptorSet> writeDescriptorSets = { writeDescriptorSet };
		vkUpdateDescriptorSets(Device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

		// Pipeline cache
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VK_ERROR(vkCreatePipelineCache(Device, &pipelineCacheCreateInfo, nullptr, &ImguiPipelineCache));

		// Pipeline layout
		// Push constants for UI rendering parameters
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstBlock);

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &ImguiLayout;

		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
		VK_ERROR(vkCreatePipelineLayout(Device, &pipelineLayoutCreateInfo, nullptr, &ImguiPipelineLayout));

		// Setup graphics pipeline for UI rendering
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyState.flags = 0;
		inputAssemblyState.primitiveRestartEnable = VK_FALSE;

		VkPipelineRasterizationStateCreateInfo rasterizationState{};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.flags = 0;
		rasterizationState.depthClampEnable = VK_FALSE;
		rasterizationState.lineWidth = 1.0f;

		// Enable blending
		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &blendAttachmentState;

		VkPipelineDepthStencilStateCreateInfo depthStencilState{};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable = VK_FALSE;
		depthStencilState.depthWriteEnable = VK_FALSE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
		viewportState.flags = 0;

		VkPipelineMultisampleStateCreateInfo multisampleState{};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleState.flags = 0;

		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pDynamicStates = dynamicStateEnables.data();
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
		dynamicState.flags = 0;

		VkPipelineShaderStageCreateInfo shaderStages[2] = { };

		VkRenderPass renderPass;
		vlk.GetRenderPass((void**)&renderPass);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.layout = ImguiPipelineLayout;
		pipelineCreateInfo.renderPass = renderPass;
		pipelineCreateInfo.flags = 0;
		pipelineCreateInfo.basePipelineIndex = -1;
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = 2;

		// Vertex bindings an attributes based on ImGui vertex definition
		VkVertexInputBindingDescription vertexInputBinding{};
		vertexInputBinding.binding = 0;
		vertexInputBinding.stride = sizeof(ImDrawVert);
		vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		std::vector<VkVertexInputBindingDescription> vertexInputBindings = { vertexInputBinding };

		VkVertexInputAttributeDescription vInputAttribDescription1{};
		vInputAttribDescription1.location = 0;
		vInputAttribDescription1.binding = 0;
		vInputAttribDescription1.format = VK_FORMAT_R32G32_SFLOAT;
		vInputAttribDescription1.offset = offsetof(ImDrawVert, pos); // Location 0: Position

		VkVertexInputAttributeDescription vInputAttribDescription2{};
		vInputAttribDescription2.location = 1;
		vInputAttribDescription2.binding = 0;
		vInputAttribDescription2.format = VK_FORMAT_R32G32_SFLOAT;
		vInputAttribDescription2.offset = offsetof(ImDrawVert, uv); // Location 1: UV

		VkVertexInputAttributeDescription vInputAttribDescription3{};
		vInputAttribDescription3.location = 2;
		vInputAttribDescription3.binding = 0;
		vInputAttribDescription3.format = VK_FORMAT_R8G8B8A8_UNORM;
		vInputAttribDescription3.offset = offsetof(ImDrawVert, col); // Location 2: Color

		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
			vInputAttribDescription1,
			vInputAttribDescription2,
			vInputAttribDescription3
		};

		VkPipelineVertexInputStateCreateInfo vertexInputState{};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
		vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

		pipelineCreateInfo.pVertexInputState = &vertexInputState;

		// Intialize runtime shader compiler HLSL -> SPIRV
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
		// TODO: Part 3C
		shaderc_compile_options_set_invert_y(options, false);
#ifndef NDEBUG
		shaderc_compile_options_set_generate_debug_info(options);
#endif
		std::string VertexShaderImGuiource = FileHelper::LoadShaderFileIntoString("../Shaders/ImGuiVertex.hlsl");
		std::string PixelShaderImGuiource = FileHelper::LoadShaderFileIntoString("../Shaders/ImGuiPixel.hlsl");

		// Create Vertex Shader
		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
			compiler, VertexShaderImGuiource.c_str(), VertexShaderImGuiource.length(),
			shaderc_vertex_shader, "main.vert", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &VertexShader_ImGui);
		shaderc_result_release(result); // done
		// Create Pixel Shader
		result = shaderc_compile_into_spv( // compile
			compiler, PixelShaderImGuiource.c_str(), PixelShaderImGuiource.length(),
			shaderc_fragment_shader, "main.frag", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &PixelShader_ImGui);
		shaderc_result_release(result); // done

		VkPipelineShaderStageCreateInfo shaderStageV = {};
		shaderStageV.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageV.stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStageV.module = VertexShader_ImGui;
		shaderStageV.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStageP = {};
		shaderStageP.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageP.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStageP.module = PixelShader_ImGui;
		shaderStageP.pName = "main";


		shaderStages[0] = shaderStageV;
		shaderStages[1] = shaderStageP;

		pipelineCreateInfo.pStages = shaderStages;

		VK_ERROR(vkCreateGraphicsPipelines(Device, ImguiPipelineCache, 1, &pipelineCreateInfo, nullptr, &ImguiPipeline));
	}

	uint32_t GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound) const
	{
		VkPhysicalDevice PhysicalDevice;
		vlk.GetPhysicalDevice((void**)&PhysicalDevice);
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memoryProperties);

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if ((typeBits & 1) == 1)
			{
				if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					if (memTypeFound)
					{
						*memTypeFound = true;
					}
					return i;
				}
			}
			typeBits >>= 1;
		}

		if (memTypeFound)
		{
			*memTypeFound = false;
			return 0;
		}
		else
		{
			throw std::runtime_error("Could not find a matching memory type");
		}
	}

private:
	void CleanUp()
	{
		// wait till everything has completed
		vkDeviceWaitIdle(device);
		ImGui::DestroyContext();

		// Release allocated buffers, shaders & pipeline
		vkDestroyShaderModule(device, VertexShader_Normal, nullptr);
		vkDestroyShaderModule(device, VertexShader_Basic, nullptr);
		vkDestroyShaderModule(device, VertexShader_Debug, nullptr);
		vkDestroyShaderModule(device, PixelShader_Basic, nullptr);
		vkDestroyShaderModule(device, PixelShader_Debug, nullptr);
		vkDestroyShaderModule(device, PixelShader_Normal, nullptr);
		vkDestroyShaderModule(device, PixelShader_Toon, nullptr);
		vkDestroyShaderModule(device, PixelShader_Fresnel, nullptr);
		vkDestroyShaderModule(device, PixelShader_FresnelNormal, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipeline(device, Pipeline_Normal, nullptr);
		vkDestroyPipeline(device, Pipeline_Toon, nullptr);
		vkDestroyPipeline(device, Pipeline_Fresnel, nullptr);
		vkDestroyPipeline(device, Pipeline_FresnelNormal, nullptr);
		vkDestroyPipeline(device, Pipeline_Debug, nullptr);
		vkDestroyPipeline(device, Pipeline_Debug2, nullptr);

		// ImGui
		vkDestroyImage(device, fontImage, nullptr);
		vkDestroyImageView(device, fontView, nullptr);
		vkFreeMemory(device, fontMemory, nullptr);

		vkDestroySampler(device, ImguiSampler, nullptr);

		vkDestroyDescriptorPool(device, ImguiDescpool, nullptr);
		vkDestroyDescriptorSetLayout(device, ImguiLayout, nullptr);

		vkDestroyPipelineCache(device, ImguiPipelineCache, nullptr);
		vkDestroyPipelineLayout(device, ImguiPipelineLayout, nullptr);

		vkDestroyPipeline(device, ImguiPipeline, nullptr);

		vkDestroyShaderModule(device, VertexShader_ImGui, nullptr);
		vkDestroyShaderModule(device, PixelShader_ImGui, nullptr);

		vkDestroyBuffer(device, ImguiVertexBuffer, nullptr);
		vkDestroyBuffer(device, ImguiIndexBuffer, nullptr);

		vkFreeMemory(device, ImguiVertexBufferData, nullptr);
		vkFreeMemory(device, ImguiIndexBufferData, nullptr);

		if (World)
		{
			delete World;
		}
	}
};
