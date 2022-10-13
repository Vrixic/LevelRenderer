// minimalistic code to draw a single triangle, this is not part of the API.
#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#ifdef _WIN32 // must use MT platform DLL libraries on windows
#pragma comment(lib, "shaderc_combined.lib") 
#endif

#include "Math/Matrix4D.h"
#include "FileHelper.h"
#include "Level.h"

// Creation, Rendering & Cleanup
class Renderer
{
	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GVulkanSurface vlk;
	GW::CORE::GEventReceiver shutdown;
	// TODO: Part 4a
	GW::INPUT::GInput Input;
	GW::INPUT::GController Controller;

	float TimePassed;
	// TODO: Part 2a
	GW::MATH::GMatrix Matrix;
	//GW::MATH::GMATRIXF GridWorldMatrices[6];
	// TODO: Part 3d
	// TODO: Part 2e
	// TODO: Part 3a
	//GW::MATH::GMATRIXF ViewProjectionMatrix;
	// what we need at a minimum to draw a triangle
	VkDevice device = nullptr;
	//VkBuffer vertexHandle = nullptr;
	//VkDeviceMemory vertexData = nullptr;
	VkShaderModule vertexShader = nullptr;
	VkShaderModule pixelShader = nullptr;
	// pipeline settings for drawing (also required)
	VkPipeline pipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
public:
	// TODO: Part 1c -> Vertex struct
	//struct Vertex
	//{
	//	float Position[4];
	//};
	// TODO: Part 2b -> ShaderVariables struct 
	//struct ShaderVars
	//{
	//	GW::MATH::GMATRIXF World;
	//	GW::MATH::GMATRIXF View;
	//};
	// TODO: Part 2f
	//GW::MATH::GMATRIXF GridViewMatrix;

	Level* World;

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
		//for (int i = 0; i < 6; ++i)
		//{
		//	GMatrix::IdentityF(GridWorldMatrices[i]);
		//}

		//GVECTORF TranslateVector;
		//TranslateVector.x = 0.0f;
		//TranslateVector.y = -0.5f;
		//TranslateVector.z = 0.0f;
		//TranslateVector.w = 0.0f;

		//Matrix.TranslateLocalF(GridWorldMatrices[0], TranslateVector, GridWorldMatrices[0]);
		//Matrix.RotateXLocalF(GridWorldMatrices[0], Math::DegreesToRadians(90.0f), GridWorldMatrices[0]);

		//// TODO: Part 3d
		//// Ceil
		//TranslateVector.y = 0.5f;
		//Matrix.TranslateLocalF(GridWorldMatrices[1], TranslateVector, GridWorldMatrices[1]);
		//Matrix.RotateXLocalF(GridWorldMatrices[1], Math::DegreesToRadians(90.0f), GridWorldMatrices[1]);

		//// Walls
		//TranslateVector.y = 0.0f;

		//TranslateVector.z = 0.5f;
		//Matrix.TranslateLocalF(GridWorldMatrices[2], TranslateVector, GridWorldMatrices[2]);

		//TranslateVector.z = -0.5f;
		//Matrix.TranslateLocalF(GridWorldMatrices[3], TranslateVector, GridWorldMatrices[3]);

		//TranslateVector.z = 0.0f;

		//TranslateVector.x = 0.5f;
		//Matrix.TranslateLocalF(GridWorldMatrices[4], TranslateVector, GridWorldMatrices[4]);
		//Matrix.RotateYLocalF(GridWorldMatrices[4], Math::DegreesToRadians(90.0f), GridWorldMatrices[4]);

		//TranslateVector.x = -0.5f;
		//Matrix.TranslateLocalF(GridWorldMatrices[5], TranslateVector, GridWorldMatrices[5]);
		//Matrix.RotateYLocalF(GridWorldMatrices[5], Math::DegreesToRadians(90.0f), GridWorldMatrices[5]);

		//// TODO: Part 2e -> Create the view matrix
		//Matrix.IdentityF(GridViewMatrix);
		//GVECTORF Eye = { 0.25, -0.125, -0.25, 1.0 };
		//GVECTORF At = { 0.0, -0.5, 0.0, 1.0 };
		//GVECTORF Up = { 0.0, 1.0, 0.0, 1.0 };

		//Matrix.LookAtLHF(Eye, At, Up, GridViewMatrix);

		/***************** GEOMETRY INTIALIZATION ******************/
		// Grab the device & physical device so we can allocate some stuff
		VkPhysicalDevice physicalDevice = nullptr;
		vlk.GetDevice((void**)&device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);

		World = new Level(&device, &vlk, &pipelineLayout, "Vrixic", "../GameLevel.txt");

		// Create Vertex Buffer
		/*Vertex verts[104] = { 0 };

		float VertX = 0.5f;
		float VertY = 0.5f;
		float VertZ = 0.0f;
		float VertW = 1.0f;

		for (int i = 0; i < 104; i += 2)
		{
			if (i < 52)
			{
				float VertYSub = i / 50.0f;

				verts[i] = { {VertX, VertY - VertYSub, VertZ, VertW} };
				verts[i + 1] = { {-VertX, VertY - VertYSub, VertZ, VertW} };
			}
			else
			{
				float VertXSub = (i - 52.0f) / 50.0f;

				verts[i] = { {VertX - VertXSub, VertY, VertZ, VertW} };
				verts[i + 1] = { {VertX - VertXSub, -VertY, VertZ, VertW} };
			}
		}*/

		// Transfer triangle data to the vertex buffer. (staging would be prefered here)
		/*GvkHelper::create_buffer(physicalDevice, device, sizeof(verts),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexHandle, &vertexData);
		GvkHelper::write_to_buffer(device, vertexData, verts, sizeof(verts));*/

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
		std::string VertexShaderSource = FileHelper::LoadShaderFileIntoString("../Shaders/ModelVertex.hlsl");
		std::string PixelShaderSource = FileHelper::LoadShaderFileIntoString("../Shaders/ModelPixel.hlsl");

		// Create Vertex Shader
		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
			compiler, VertexShaderSource.c_str(), VertexShaderSource.length(),
			shaderc_vertex_shader, "main.vert", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &vertexShader);
		shaderc_result_release(result); // done
		// Create Pixel Shader
		result = shaderc_compile_into_spv( // compile
			compiler, PixelShaderSource.c_str(), PixelShaderSource.length(),
			shaderc_fragment_shader, "main.frag", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &pixelShader);
		shaderc_result_release(result); // done
		// Free runtime shader compiler resources
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);

		/***************** PIPELINE INTIALIZATION ******************/
		// Create Pipeline & Layout (Thanks Tiny!)
		VkRenderPass renderPass;
		vlk.GetRenderPass((void**)&renderPass);
		VkPipelineShaderStageCreateInfo stage_create_info[2] = {};
		// Create Stage Info for Vertex Shader
		stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage_create_info[0].module = vertexShader;
		stage_create_info[0].pName = "main";
		// Create Stage Info for Fragment Shader
		stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage_create_info[1].module = pixelShader;
		stage_create_info[1].pName = "main";
		// Assembly State
		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = {};
		assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		assembly_create_info.primitiveRestartEnable = false;
		// Vertex Input State
		// TODO: Part 1c
		VkVertexInputBindingDescription vertex_binding_description = {};
		vertex_binding_description.binding = 0;
		vertex_binding_description.stride = sizeof(Vertex);
		vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		// TODO: Part 1c
		VkVertexInputAttributeDescription vertex_attribute_description[4] = {
			{ 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 },
			{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, 8 },
			{ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, 20 },
			{ 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 32 }
		};
		VkPipelineVertexInputStateCreateInfo input_vertex_info = {};
		input_vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		input_vertex_info.vertexBindingDescriptionCount = 1;
		input_vertex_info.pVertexBindingDescriptions = &vertex_binding_description;
		input_vertex_info.vertexAttributeDescriptionCount = 4;
		input_vertex_info.pVertexAttributeDescriptions = vertex_attribute_description;
		// Viewport State (we still need to set this up even though we will overwrite the values)
		VkViewport viewport = {
			0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
		};
		VkRect2D scissor = { {0, 0}, {width, height} };
		VkPipelineViewportStateCreateInfo viewport_create_info = {};
		viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_create_info.viewportCount = 1;
		viewport_create_info.pViewports = &viewport;
		viewport_create_info.scissorCount = 1;
		viewport_create_info.pScissors = &scissor;
		// Rasterizer State
		VkPipelineRasterizationStateCreateInfo rasterization_create_info = {};
		rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;
		rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization_create_info.lineWidth = 1.0f;
		rasterization_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterization_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterization_create_info.depthClampEnable = VK_FALSE;
		rasterization_create_info.depthBiasEnable = VK_FALSE;
		rasterization_create_info.depthBiasClamp = 0.0f;
		rasterization_create_info.depthBiasConstantFactor = 0.0f;
		rasterization_create_info.depthBiasSlopeFactor = 0.0f;
		// Multisampling State
		VkPipelineMultisampleStateCreateInfo multisample_create_info = {};
		multisample_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_create_info.sampleShadingEnable = VK_FALSE;
		multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisample_create_info.minSampleShading = 1.0f;
		multisample_create_info.pSampleMask = VK_NULL_HANDLE;
		multisample_create_info.alphaToCoverageEnable = VK_FALSE;
		multisample_create_info.alphaToOneEnable = VK_FALSE;
		// Depth-Stencil State
		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {};
		depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_create_info.depthTestEnable = VK_TRUE;
		depth_stencil_create_info.depthWriteEnable = VK_TRUE;
		depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
		depth_stencil_create_info.depthBoundsTestEnable = VK_FALSE;
		depth_stencil_create_info.minDepthBounds = 0.0f;
		depth_stencil_create_info.maxDepthBounds = 1.0f;
		depth_stencil_create_info.stencilTestEnable = VK_FALSE;
		// Color Blending Attachment & State
		VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
		color_blend_attachment_state.colorWriteMask = 0xF;
		color_blend_attachment_state.blendEnable = VK_FALSE;
		color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
		color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
		color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
		VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
		color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_create_info.logicOpEnable = VK_FALSE;
		color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
		color_blend_create_info.attachmentCount = 1;
		color_blend_create_info.pAttachments = &color_blend_attachment_state;
		color_blend_create_info.blendConstants[0] = 0.0f;
		color_blend_create_info.blendConstants[1] = 0.0f;
		color_blend_create_info.blendConstants[2] = 0.0f;
		color_blend_create_info.blendConstants[3] = 0.0f;
		// Dynamic State 
		VkDynamicState dynamic_state[2] = {
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamic_create_info = {};
		dynamic_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_create_info.dynamicStateCount = 2;
		dynamic_create_info.pDynamicStates = dynamic_state;
		// TODO: Part 2c
		//VkPushConstantRange PushConstantRange = { };
		//PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		//PushConstantRange.offset = 0;
		//PushConstantRange.size = sizeof(ShaderVars);

		/* Load World Before pipeline creation */
		World->Load();

		std::vector<VkDescriptorSetLayout*> DescSetLayouts = World->GetShaderStorageDescSetLayouts();

		// Descriptor pipeline layout
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = { };
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.pNext = nullptr;
		pipeline_layout_create_info.setLayoutCount = DescSetLayouts.size();
		pipeline_layout_create_info.pSetLayouts = *DescSetLayouts.data();

		pipeline_layout_create_info.flags = 0;

		pipeline_layout_create_info.pushConstantRangeCount = 0; // TODO: Part 2d 
		pipeline_layout_create_info.pPushConstantRanges = nullptr; // TODO: Part 2d

		vkCreatePipelineLayout(device, &pipeline_layout_create_info,
			nullptr, &pipelineLayout);
		// Pipeline State... (FINALLY) 
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = 2;
		pipeline_create_info.pStages = stage_create_info;
		pipeline_create_info.pInputAssemblyState = &assembly_create_info;
		pipeline_create_info.pVertexInputState = &input_vertex_info;
		pipeline_create_info.pViewportState = &viewport_create_info;
		pipeline_create_info.pRasterizationState = &rasterization_create_info;
		pipeline_create_info.pMultisampleState = &multisample_create_info;
		pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
		pipeline_create_info.pColorBlendState = &color_blend_create_info;
		pipeline_create_info.pDynamicState = &dynamic_create_info;
		pipeline_create_info.layout = pipelineLayout;
		pipeline_create_info.renderPass = renderPass;
		pipeline_create_info.subpass = 0;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
			&pipeline_create_info, nullptr, &pipeline);

		

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
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		// TODO: Part 3a -> Projection matrix
		GW::MATH::GMATRIXF ProjectionMatrix;
		GW::MATH::GMatrix::IdentityF(ProjectionMatrix);
		float FOV = Math::DegreesToRadians(65);
		float NearPlane = 0.1f;
		float FarPlane = 100.0f;
		float AspectRatio = 0.0f;
		vlk.GetAspectRatio(AspectRatio);
		GW::MATH::GMatrix::ProjectionDirectXLHF(FOV, AspectRatio, NearPlane, FarPlane, ProjectionMatrix);

		World->Bind();
		//vkCmdDrawIndexed(commandBuffer, 1560, 1, 11755, 6142, 0 );
		vkCmdDrawIndexed(commandBuffer, 11754, 1, 0, 0, 0);

		// TODO: Part 3b -> View projection matrix 
		//GW::MATH::GMatrix::MultiplyMatrixF(GridViewMatrix, ProjectionMatrix, ViewProjectionMatrix);
		// TODO: Part 2b -> Shader constant buffer instance
		//ShaderVars ConstantBuffer;
		//ConstantBuffer.World = GridWorldMatrices[0];
		// TODO: Part 2f, Part 3b
		//ConstantBuffer.View = ViewProjectionMatrix;
		// TODO: Part 2d -> Pushing/Sending shader constant buffer from cpu to gpu
		//vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShaderVars), &ConstantBuffer);
		// now we can draw
		//VkDeviceSize offsets[] = { 0 };
		//vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexHandle, offsets);
		//vkCmdDraw(commandBuffer, 104, 1, 0, 0); // TODO: Part 1b // TODO: Part 1c
		// TODO: Part 3e
		/*for (int i = 1; i < 6; ++i)
		{
			ConstantBuffer.World = GridWorldMatrices[i];
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShaderVars), &ConstantBuffer);
			vkCmdDraw(commandBuffer, 104, 1, 0, 0);

		}*/
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
		float CameraRotationSpeed = 10.0f;

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

		if (Input.GetMouseDelta(MouseDeltaX, MouseDeltaY) != GW::GReturn::SUCCESS)
		{
			MouseDeltaX = 0;
			MouseDeltaY = 0;
		};

		Controller.GetState(0, G_RIGHT_TRIGGER_AXIS, RightTriggerState);
		Controller.GetState(0, G_LEFT_TRIGGER_AXIS, LeftTriggerState);
		Controller.GetState(0, G_LY_AXIS, LeftStickYAxis);
		Controller.GetState(0, G_LX_AXIS, LeftStickXAxis);
		Controller.GetState(0, G_RY_AXIS, RightStickYAxis);
		Controller.GetState(0, G_RX_AXIS, RightStickXAxis);

		GMATRIXF GridViewMatrix = World->GetViewMatrix();

		// Calculate The Deltas 		
		float DeltaX = DKeyState - AKeyState + LeftStickXAxis;
		float DeltaY = (SpaceKeyState - LeftShiftKeyState) + (RightTriggerState - LeftTriggerState);
		float DeltaZ = WKeyState - SKeyState + LeftStickYAxis;
		float ThumbSpeed = PI * TimePassed;

		float TotalPitch = (Math::DegreesToRadians(65.0f) * MouseDeltaY / ScreenHeight + RightStickYAxis * -ThumbSpeed) * CameraRotationSpeed;
		float TotalYaw = (Math::DegreesToRadians(65.0f) * MouseDeltaX / ScreenWidth + RightStickXAxis * ThumbSpeed) * CameraRotationSpeed;

		// Camera Variables
		const float CameraSpeed = 10.0f;
		float PerFrameSpeed = TimePassed * CameraSpeed;

		// TODO: Part 4c
		Matrix.InverseF(GridViewMatrix, GridViewMatrix);

		// TODO: Part 4d -> Camera Movement Y
		GMATRIXF CameraTranslationMatrix;
		Matrix.IdentityF(CameraTranslationMatrix);

		GW::MATH::GVECTORF CameraTranslateVector = { 0.0f, 0.0f, 0.0f, 1.0f };
		CameraTranslateVector.y += DeltaY * PerFrameSpeed;
		Matrix.TranslateGlobalF(CameraTranslationMatrix, CameraTranslateVector, CameraTranslationMatrix);
		Matrix.MultiplyMatrixF(GridViewMatrix, CameraTranslationMatrix, GridViewMatrix);
		Matrix.IdentityF(CameraTranslationMatrix);
		CameraTranslateVector.y = 0;
		// TODO: Part 4e -> Camera Movement XZ
		CameraTranslateVector.x = DeltaX * PerFrameSpeed;
		CameraTranslateVector.z = DeltaZ * PerFrameSpeed;
		Matrix.TranslateLocalF(CameraTranslationMatrix, CameraTranslateVector, CameraTranslationMatrix);
		Matrix.MultiplyMatrixF(CameraTranslationMatrix, GridViewMatrix, GridViewMatrix);
		// TODO: Part 4f -> Pitch Rotation 
		GMATRIXF PitchMatrix;
		Matrix.IdentityF(PitchMatrix);
		Matrix.RotateXLocalF(PitchMatrix, Math::DegreesToRadians(TotalPitch), PitchMatrix);
		Matrix.MultiplyMatrixF(PitchMatrix, GridViewMatrix, GridViewMatrix);
		// TODO: Part 4g -> Yaw Rotation
		GMATRIXF YawMatrix;
		Matrix.IdentityF(YawMatrix);
		Matrix.RotateYGlobalF(YawMatrix, Math::DegreesToRadians(TotalYaw), YawMatrix);
		GVECTORF CameraPosition = GridViewMatrix.row4;
		Matrix.MultiplyMatrixF(GridViewMatrix, YawMatrix, GridViewMatrix);
		GridViewMatrix.row4 = CameraPosition;

		// TODO: Part 4c
		Matrix.InverseF(GridViewMatrix, GridViewMatrix);

		World->SetViewMatrix(GridViewMatrix);

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
		//vkDestroyBuffer(device, vertexHandle, nullptr);
		//vkFreeMemory(device, vertexData, nullptr);
		vkDestroyShaderModule(device, vertexShader, nullptr);
		vkDestroyShaderModule(device, pixelShader, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);

		if (World)
		{
			delete World;
		}
	}
};
