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

struct ConstantBuffer
{
	uint32 MeshID;
	uint32 MaterialID;
	uint32 DiffuseTextureID;
	uint32 ViewMatID;

	Vector3D Color;
	uint32 SpecularTextureID;
};

#define GREEN Vector3D(0,1,0)
#define RED Vector3D(1,0,0)
#define BLUE Vector3D(0,0,1)

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
	VkShaderModule VertexShader_Texture = nullptr;
	VkShaderModule VertexShader_Basic = nullptr;
	VkShaderModule VertexShader_Debug = nullptr;
	VkShaderModule VertexShader_Material = nullptr;
	VkShaderModule PixelShader_Texture = nullptr;
	VkShaderModule PixelShader_Basic = nullptr;;
	VkShaderModule PixelShader_Debug = nullptr;;
	VkShaderModule PixelShader_Material = nullptr;

	// pipeline settings for drawing (also required)
	VkPipeline Pipeline_Texture = nullptr;
	VkPipeline Pipeline_Material = nullptr;
	VkPipeline Pipeline_Debug = nullptr;
	VkPipeline Pipeline_Debug2 = nullptr;
	VkPipeline* CurrentPipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;

	VulkanPipeline PipelineCreator;

	bool CaptureInput = true;
	float CameraSpeed = 5.0f;

	Frustum CameraFrustum;

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

		/***************** GEOMETRY INTIALIZATION ******************/
		// Grab the device & physical device so we can allocate some stuff
		VkPhysicalDevice physicalDevice = nullptr;
		vlk.GetDevice((void**)&device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);

		World = new Level(&device, &vlk, &pipelineLayout, "Vrixic", "../Levels/TestLevel_Textures_3.txt");

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
		std::string VertexShaderTextureSource = FileHelper::LoadShaderFileIntoString("../Shaders/TextureVertex.hlsl");
		std::string PixelShaderTextureSource = FileHelper::LoadShaderFileIntoString("../Shaders/TexturePixel.hlsl");

		// Create Vertex Shader
		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
			compiler, VertexShaderTextureSource.c_str(), VertexShaderTextureSource.length(),
			shaderc_vertex_shader, "main.vert", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &VertexShader_Texture);
		shaderc_result_release(result); // done
		// Create Pixel Shader
		result = shaderc_compile_into_spv( // compile
			compiler, PixelShaderTextureSource.c_str(), PixelShaderTextureSource.length(),
			shaderc_fragment_shader, "main.frag", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &PixelShader_Texture);
		shaderc_result_release(result); // done

		std::string VertexShaderModelSource = FileHelper::LoadShaderFileIntoString("../Shaders/MaterialVertex.hlsl");
		std::string PixelShaderModelSource = FileHelper::LoadShaderFileIntoString("../Shaders/MaterialPixel.hlsl");

		// Create Vertex Shader
		result = shaderc_compile_into_spv( // compile
			compiler, VertexShaderModelSource.c_str(), VertexShaderModelSource.length(),
			shaderc_vertex_shader, "main.vert", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &VertexShader_Material);
		shaderc_result_release(result); // done
		// Create Pixel Shader
		result = shaderc_compile_into_spv( // compile
			compiler, PixelShaderModelSource.c_str(), PixelShaderModelSource.length(),
			shaderc_fragment_shader, "main.frag", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &PixelShader_Material);
		shaderc_result_release(result); // done

		std::string VertexShaderBasicSource = FileHelper::LoadShaderFileIntoString("../Shaders/BasicVertex.hlsl");
		std::string PixelShaderBasicSource = FileHelper::LoadShaderFileIntoString("../Shaders/BasicPixel.hlsl");

		// Create Vertex Shader
		result = shaderc_compile_into_spv( // compile
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

		// Free runtime shader compiler resources
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);

		/***************** PIPELINE INTIALIZATION ******************/
		// Create Pipeline & Layout (Thanks Tiny!)
		VkRenderPass renderPass;
		vlk.GetRenderPass((void**)&renderPass);

		// Create Stage Info for Vertex Shader
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, &VertexShader_Texture);
		// Create Stage Info for Fragment Shader
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, &PixelShader_Texture);

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

		// Second viewport 
		//Pos.X += 10.0f;
		//Pos.Y += 10.0f;
		//WidthHeight.X = 100.0f;
		//WidthHeight.Y = 100.0f;
		//
		//PipelineCreator.AddNewViewport(Pos, WidthHeight, MinMaxDepth);
		//PipelineCreator.AddNewScissor(0, 0, 100, 100);

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
		FileHelper::ReadGameLevelFile("../Levels/TestLevel_Textures_3.txt", RawData);

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
		Data.Materials.push_back(RawData[1].Materials[0]); // random mat

		Data.WorldMatrices.push_back(Matrix4D::Identity());

		H2B::MESH M;
		M.materialIndex = 0;
		M.name = "nullptr";
		M.drawInfo.indexCount = Frustum::GetFrustumIndexCount();
		M.drawInfo.indexOffset = 0;

		Data.Meshes.push_back(M);

		RawData.push_back(Data);

		StaticMeshes = World->Load(RawData);

		DebugMeshes.push_back(StaticMeshes[StaticMeshes.size() - 1]);
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
		PipelineCreator.CreateGraphicsPipelines(&device, Pipeline_Texture);

		/* Reset the layout create info expect this time we dont want textures
		* TODO: Not make this hardcoded size to one
		*/
		PipelineCreator.SetLayoutCreateInfo(1, *DescSetLayouts.data());
		PipelineCreator.ClearStageCreateInfos();
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, &VertexShader_Material);
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, &PixelShader_Material);
		PipelineCreator.CreateGraphicsPipelines(&device, Pipeline_Material);

		PipelineCreator.SetInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
		PipelineCreator.ClearStageCreateInfos();
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, &VertexShader_Basic);
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, &PixelShader_Basic);
		PipelineCreator.CreateGraphicsPipelines(&device, Pipeline_Debug);

		PipelineCreator.SetInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
		PipelineCreator.ClearStageCreateInfos();
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, &VertexShader_Debug);
		PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, &PixelShader_Debug);
		PipelineCreator.CreateGraphicsPipelines(&device, Pipeline_Debug2);


		/* check is textures are supported if not */
		for (uint32 i = 0; i < StaticMeshes.size(); ++i)
		{
			if (StaticMeshes[i].SupportsTextures())
			{
				SceneHasTextures = true;
				break;
			}
		}

		if (SceneHasTextures)
		{
			CurrentPipeline = &Pipeline_Texture;
		}
		else
		{
			CurrentPipeline = &Pipeline_Material;
		}

		CameraView1 = World->GetViewMatrix1();
		CameraView2 = World->GetViewMatrix2();
		CameraView3 = World->GetViewMatrix3();

		/***************** CLEANUP / SHUTDOWN ******************/
		// GVulkanSurface will inform us when to release any allocated resources
		shutdown.Create(vlk, [&]() {
			if (+shutdown.Find(GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES, true)) {
				CleanUp(); // unlike D3D we must be careful about destroy timing
			}
			});
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


		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *CurrentPipeline);
		World->Bind();

		/*--------------------------------------------------DEBUG-------------------------------------------------------*/
		ConstantBuffer Buffer;

		for (uint32 i = 0; i < RenderMeshes.size(); ++i)
		{
			Buffer.MeshID = RenderMeshes[i].GetWorldMatrixIndex();

			for (uint32 j = 0; j < RenderMeshes[i].GetMeshCount(); ++j)
			{
				Buffer.MaterialID = RenderMeshes[i].GetMaterialIndex() + RenderMeshes[i].GetSubMeshMaterialIndex(j);
				Buffer.DiffuseTextureID = RenderMeshes[i].GetSubMeshDiffuseTextureIndex(j);
				Buffer.SpecularTextureID = RenderMeshes[i].GetSubMeshSpecularTextureIndex(j);
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

		viewport.x = 50;
		viewport.y = 50;
		viewport.width = 200;
		viewport.height = 200;
		scissor.offset.x = 50;
		scissor.offset.y = 50;
		scissor.extent.width = 200;
		scissor.extent.height = 200;
		
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		
		for (uint32 i = 0; i < RenderMeshes.size(); ++i)
		{
			Buffer.MeshID = RenderMeshes[i].GetWorldMatrixIndex();
		
			for (uint32 j = 0; j < RenderMeshes[i].GetMeshCount(); ++j)
			{
				Buffer.MaterialID = RenderMeshes[i].GetMaterialIndex() + RenderMeshes[i].GetSubMeshMaterialIndex(j);
				Buffer.DiffuseTextureID = RenderMeshes[i].GetSubMeshDiffuseTextureIndex(j);
				Buffer.SpecularTextureID = RenderMeshes[i].GetSubMeshSpecularTextureIndex(j);
				Buffer.ViewMatID = 1;
		
				vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT |
					VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ConstantBuffer), &Buffer);
				vkCmdDrawIndexed(commandBuffer,
					RenderMeshes[i].GetSubMeshIndexCount(j),
					RenderMeshes[i].GetInstanceCount(),
					RenderMeshes[i].GetIndexOffset() + RenderMeshes[i].GetSubMeshIndexOffset(j),
					RenderMeshes[i].GetVertexOffset(), 0);
			}
		}
		
		/* Draw AABBS */
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline_Debug);
		
		for (uint32 i = 0; i < StaticMeshes.size(); ++i)
		{
			Buffer.MeshID = StaticMeshes[i].GetWorldMatrixIndex();
			Buffer.ViewMatID = 1;
			Buffer.Color = StaticMeshes[i].Color_AABB;
		
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT |
				VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ConstantBuffer), &Buffer);
			vkCmdDrawIndexed(commandBuffer,
				24,
				StaticMeshes[i].GetInstanceCount(),
				World->GetLevelData()->DebugBoxIndexStart,
				World->GetLevelData()->DebugBoxVertexStart + (i * 8), 0);
		}
		
		Buffer.MeshID = DebugMeshes[0].GetWorldMatrixIndex();
		Buffer.ViewMatID = 1;
		Buffer.Color = Vector3D::ZeroVector();
		
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT |
			VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ConstantBuffer), &Buffer);
		vkCmdDrawIndexed(commandBuffer,
			Frustum::GetFrustumIndexCount(),
			1,
			World->GetLevelData()->DebugBoxIndexStart - 24,
			World->GetLevelData()->DebugBoxVertexStart - 8, 0);
		
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
				World->GetLevelData()->DebugBoxIndexStart + 24,
				World->GetLevelData()->DebugBoxVertexStart + (i * 8), 0);
		}
		
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
		for (uint32 i = Vertices->size() - 12; i < Vertices->size(); i+=2)
		{
			Buffer.Color = Vector3D((*Vertices)[i].Color.X, (*Vertices)[i].Color.Y, (*Vertices)[i].Color.Z);
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT |
				VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ConstantBuffer), &Buffer);
			vkCmdDraw(commandBuffer, 2, 1, i, 0);
		}

		/*--------------------------------------------------DEBUG-------------------------------------------------------*/
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
		float UKeyState = 0;
		float OneKeyState = 0;
		float TwoKeyState = 0;
		float ThreeKeyState = 0;
		float CameraRotationSpeed = 50.0f;

		float MouseScrollUp = 0, MouseScrollDown = 0;

		// Get Screen Width/Height
		win.GetWidth(ScreenWidth);
		win.GetHeight(ScreenHeight);

		// Get The Key/Controller States
		Input.GetState(G_KEY_SPACE, SpaceKeyState);
		Input.GetState(G_KEY_LEFTSHIFT, LeftShiftKeyState);

		Input.GetState(G_KEY_W, WKeyState);
		Input.GetState(G_KEY_A, AKeyState);
		Input.GetState(G_KEY_S, SKeyState);
		Input.GetState(G_KEY_D, DKeyState);
		Input.GetState(G_KEY_F, FKeyState);
		Input.GetState(G_KEY_1, OneKeyState);
		Input.GetState(G_KEY_2, TwoKeyState);
		Input.GetState(G_KEY_3, ThreeKeyState);
		
		if (FKeyState > 0)
		{
			std::string FilePath;
			if (FileHelper::OpenFileDialog(FilePath))
			{
				//std::cout << "\n" << FilePath << "\n";

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
					Data.Materials.push_back(RawData[1].Materials[0]); // random mat

					Data.WorldMatrices.push_back(Matrix4D::Identity());

					H2B::MESH M;
					M.materialIndex = 0;
					M.name = "nullptr";
					M.drawInfo.indexCount = Frustum::GetFrustumIndexCount();
					M.drawInfo.indexOffset = 0;

					Data.Meshes.push_back(M);

					RawData.push_back(Data);

					StaticMeshes = World->Load(RawData);

					DebugMeshes.push_back(StaticMeshes[StaticMeshes.size() - 1]);
					StaticMeshes.pop_back();

					SceneHasTextures = false;
					for (uint32 i = 0; i < StaticMeshes.size(); ++i)
					{
						if (StaticMeshes[i].SupportsTextures())
						{
							SceneHasTextures = true;
							break;
						}
					}
				}

				CameraView1 = World->GetViewMatrix1();
				CameraView2 = World->GetViewMatrix2();
				CameraView3 = World->GetViewMatrix3();

				//StaticMeshes = World->Load();

				{
					if (SceneHasTextures)
					{
						VkRenderPass renderPass;
						vlk.GetRenderPass((void**)&renderPass);

						vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
						vkDestroyPipeline(device, Pipeline_Texture, nullptr);

						std::vector<VkDescriptorSetLayout*> DescSetLayouts = World->GetShaderStorageDescSetLayouts();
						PipelineCreator.SetLayoutCreateInfo(DescSetLayouts.size(), *DescSetLayouts.data());
						PipelineCreator.CreatePipelineLayout(&device, pipelineLayout);
						PipelineCreator.SetGraphicsPipelineCreateInfo(&pipelineLayout, &renderPass);

						PipelineCreator.SetInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
						PipelineCreator.ClearStageCreateInfos();
						PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, &VertexShader_Texture);
						PipelineCreator.AddNewStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, &PixelShader_Texture);
						PipelineCreator.CreateGraphicsPipelines(&device, Pipeline_Texture);

						CurrentPipeline = &Pipeline_Texture;
					}
					else
					{
						CurrentPipeline = &Pipeline_Material;

					}

				}
			}
		}

		if (Input.GetState(G_KEY_L, LKeyState) == GW::GReturn::SUCCESS && LKeyState > 0)
		{
			CaptureInput = false;
		}

		if (Input.GetState(G_KEY_U, UKeyState) == GW::GReturn::SUCCESS && UKeyState > 0)
		{
			CaptureInput = !true;
		}

		if (!CaptureInput)
		{
			return;
		}

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

		//GMATRIXF GridViewMatrix = ViewMatrix1;
		LeftShiftKeyState = 0.0;
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
		//GW::MATH::GMATRIXF GridViewMatrix = World->GetViewMatrix1();
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
		//CamF.setCamInternals(65, AspectRatio, 1.0f, 1000.0f);
		Vector4D U = reinterpret_cast<Vector4D&>(CameraView1.row2);
		Vector4D Ri = reinterpret_cast<Vector4D&>(CameraView1.row1);
		Vector4D F = reinterpret_cast<Vector4D&>(CameraView1.row3);
		Vector4D P = reinterpret_cast<Vector4D&>(CameraView1.row4);
		//
		//
		//float hh = tanf(65.0f * 0.5f) * 100.0f;
		//float hw = hh * AspectRatio;
		//
		//float hhn = tanf(65.0f * 0.5f) * 10.0f;
		//float hwn = hhn * AspectRatio;
		//
		//Vector3D FC = Vector3D(P.X, P.Y, P.Z) + Vector3D(F.X, F.Y, F.Z) * 100.0f;
		//Vector3D NC = Vector3D(P.X, P.Y, P.Z) + Vector3D(F.X, F.Y, F.Z) * 100.0f;
		Vector3D UP = Vector3D(U.X, U.Y, U.Z);
		Vector3D Right = Vector3D(Ri.X, Ri.Y, Ri.Z);
		Vector3D Pos = Vector3D(P.X, P.Y, P.Z);
		Vector3D Forw = Vector3D(F.X, F.Y, F.Z);
		//
		//Vertex FTL = { {0,0}, {FC + (UP * hh * 0.5f) - (Right * hw * 0.5f)} };
		//Vertex FTR = { {0,0}, {FC + (UP * hh * 0.5f) + (Right * hw * 0.5f)} };
		//Vertex FBL = { {0,0}, {FC - (UP * hh * 0.5f) - (Right * hw * 0.5f)} };
		//Vertex FBR = { {0,0}, {FC - (UP * hh * 0.5f) + (Right * hw * 0.5f)} };
		//
		//
		//Vertex NTL = { {0,0}, {NC + (UP * hhn * 0.5f) - (Right * hwn * 0.5f)} };
		//Vertex NTR = { {0,0}, {NC + (UP * hhn * 0.5f) + (Right * hwn * 0.5f)} };
		//Vertex NBL = { {0,0}, {NC - (UP * hhn * 0.5f) - (Right * hwn * 0.5f)} };
		//Vertex NBR = { {0,0}, {NC - (UP * hhn * 0.5f) + (Right * hwn * 0.5f)} };
#endif
		std::vector<Vertex>* Vertices = World->GetLevelData()->GetVertices();
		CameraFrustum.SetFrustumInternals(AspectRatio, 65.0f, 5.0f, 1000.0f);
		//CameraFrustum.CreateFrustum(CameraView1, 1.0f, AspectRatio, 0.1f, 100.0f);

		CameraFrustum.CreateFrustum(Pos, Right, UP, Forw);
		CameraFrustum.DebugUpdateNormalVerts(World->GetLevelData()->GetVertices()->size() - 12, World->GetLevelData()->GetVertices());
		CameraFrustum.DebugUpdateVertices(World->GetLevelData()->DebugBoxVertexStart - Frustum::GetFrustumVertexCount(), Vertices);
		World->GetLevelData()->UpdateVertexBuffer();

		GW::MATH::GMATRIXF ProjectionMatrix;
		GW::MATH::GMatrix::IdentityF(ProjectionMatrix);
		float FOV = Math::DegreesToRadians(65);
		float NearPlane = 0.1f;
		float FarPlane = 1000.0f;
		//float AspectRatio = 0.0f;
		vlk.GetAspectRatio(AspectRatio);
		GW::MATH::GMatrix::ProjectionDirectXLHF(FOV, AspectRatio, NearPlane, FarPlane, ProjectionMatrix);
		World->SetProjectionMatrix(ProjectionMatrix);

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

		//GW::MATH::GMatrix::ProjectionVulkanLHF(FOV, AspectRatio, NearPlane, FarPlane, ProjectionMatrix);

		uint32 RemovedGPUCalls = 0;
		RenderMeshes.clear();
		for (uint32 i = 0; i < StaticMeshes.size(); ++i)
		{
			/* --> Make the box in ndc space */
			Vector4D MinBox4D = World->ShaderSceneData->WorldMatrices[StaticMeshes[i].GetWorldMatrixIndex()] * Vector4D(StaticMeshes[i].MinBox_AABB, 1.0f);	 //reinterpret_cast<Matrix4D&>(CameraView1) * reinterpret_cast<Matrix4D&>(ProjectionMatrix) * 
			Vector4D MaxBox4D = World->ShaderSceneData->WorldMatrices[StaticMeshes[i].GetWorldMatrixIndex()] * Vector4D(StaticMeshes[i].MaxBox_AABB, 1.0f);	 //reinterpret_cast<Matrix4D&>(CameraView1) * reinterpret_cast<Matrix4D&>(ProjectionMatrix) * 
			
			Vector3D MinBox = Vector3D(MinBox4D.X, MinBox4D.Y, MinBox4D.Z);
			//MinBox /= MinBox4D.W;
			Vector3D MaxBox = Vector3D(MaxBox4D.X, MaxBox4D.Y, MaxBox4D.Z);
			//MaxBox /= MaxBox4D.W;

			/* NDC space check */
			//if ((MinBox.X > 1 && MaxBox.X > 1) || (MinBox.X < -1 && MaxBox.X < -1) || (MinBox.Y > 1 && MaxBox.Y > 1) || (MinBox.Z < -1 && MaxBox.Z < -1))
			//{
			//	StaticMeshes[i].Color_AABB = RED;
			//	RemovedGPUCalls++;
			//}
			//else
			//{
			//	StaticMeshes[i].Color_AABB = GREEN;
			//	RenderMeshes.push_back(StaticMeshes[i]);
			//}

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

		if (RemovedGPUCalls > 0)
		{
			//std::cout << "\n[Frustum Culling]: removed " << RemovedGPUCalls << " gpu draw calls...\n";
		}

		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		using ms = std::chrono::duration<float, std::milli>;
		TimePassed = std::chrono::duration_cast<ms>(end - begin).count();
	}

private:
	void CleanUp()
	{
		// wait till everything has completed
		vkDeviceWaitIdle(device);
		// Release allocated buffers, shaders & pipeline
		vkDestroyShaderModule(device, VertexShader_Texture, nullptr);
		vkDestroyShaderModule(device, VertexShader_Basic, nullptr);
		vkDestroyShaderModule(device, VertexShader_Debug, nullptr);
		vkDestroyShaderModule(device, VertexShader_Material, nullptr);
		vkDestroyShaderModule(device, PixelShader_Basic, nullptr);
		vkDestroyShaderModule(device, PixelShader_Debug, nullptr);
		vkDestroyShaderModule(device, PixelShader_Texture, nullptr);
		vkDestroyShaderModule(device, PixelShader_Material, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipeline(device, Pipeline_Texture, nullptr);
		vkDestroyPipeline(device, Pipeline_Debug, nullptr);
		vkDestroyPipeline(device, Pipeline_Debug2, nullptr);
		vkDestroyPipeline(device, Pipeline_Material, nullptr);

		if (World)
		{
			delete World;
		}
	}
};
