cmake_minimum_required(VERSION 3.10)

project(LevelRenderer)

# currently using unicode in some libraries on win32 but will change soon
ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)

set(SHADERS
	Shaders/TextureVertex.hlsl
	Shaders/TexturePixel.hlsl
	Shaders/BasicVertex.hlsl
	Shaders/BasicPixel.hlsl
	Shaders/DebugVertex.hlsl
	Shaders/DebugPixel.hlsl
	Shaders/MaterialVertex.hlsl
	Shaders/MaterialPixel.hlsl
)

set(SOURCE
	main.cpp
	renderer.h
	FileHelper.h
	FileHelper.cpp
	Gateware.h
	GatewareDefine.h 
	GenericDefines.h 
	h2bParser.h 
	Level.h 
	LevelData.h 
	RawMeshData.h 
	VulkanStorageBuffer.h 
	FileHelper.cpp 
	StaticMesh.h
	VulkanPipeline.h
	Frustum.h
	
	Math/Matrix4D.h
	Math/Vector2D.h
	Math/Vector3D.h
	Math/Vector4D.h
	Math/VrixicMath.h
	Math/VrixicMath.cpp
	Math/VrixicMathDirectX.h
	Math/VrixicMathHelper.h
)

if (WIN32)
	# shaderc_combined.lib in Vulkan requires this for debug & release (runtime shader compiling)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MD")
	add_executable (LevelRenderer 
		${SOURCE}
		${SHADERS}
	)
	target_include_directories(LevelRenderer PUBLIC $ENV{VULKAN_SDK}/Include/)
	target_link_directories(LevelRenderer PUBLIC $ENV{VULKAN_SDK}/Lib/)
endif(WIN32)

if(UNIX AND NOT APPLE)
	# libshaderc_combined.a is required for runtime shader compiling
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lX11 -lshaderc_combined")
    find_package(X11)
	find_package(Vulkan REQUIRED)
    link_libraries(${X11_LIBRARIES})
    include_directories(${X11_INCLUDE_DIR})
    include_directories(${Vulkan_INCLUDE_DIR}) 
	#link_directories(${Vulkan_LIBRARY}) this is currently not working
	link_libraries(${Vulkan_LIBRARIES})
	# the path is (properly)hardcoded because "${Vulkan_LIBRARY}" currently does not 
	# return a proper path on MacOS (it has the .dynlib appended)
    link_libraries(/usr/lib/x86_64-linux-gnu/libshaderc_combined.a)
    add_executable (LevelRenderer main.cpp renderer.h)
endif(UNIX AND NOT APPLE)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fmodules -fcxx-modules")
	set(Architecture ${CMAKE_OSX_ARCHITECTURES})
	find_package(Vulkan REQUIRED)
	include_directories(${Vulkan_INCLUDE_DIR}) 
	#link_directories(${Vulkan_LIBRARY}) this is currently not working
	link_libraries(${Vulkan_LIBRARIES})
	# libshaderc_combined.a is required for runtime shader compiling
	# the path is (properly)hardcoded because "${Vulkan_LIBRARY}" currently does not 
	# return a proper path on MacOS (it has the .dynlib appended)
	link_libraries(/usr/local/lib/libshaderc_combined.a)
	add_executable (LevelRenderer main.mm)
endif(APPLE)

# add support for ktx texture loading
include_directories(${CMAKE_SOURCE_DIR}/ktx/include)

# add support for ktx texture loading
include_directories(${CMAKE_SOURCE_DIR}/imgui)

if (WIN32)
	# Find the libraries
	find_library(KTX_LIB_D NAMES ktx PATHS ${CMAKE_SOURCE_DIR}/ktx/lib/win32/Debug)
	find_library(KTX_READ_LIB_D NAMES ktx_read PATHS ${CMAKE_SOURCE_DIR}/ktx/lib/win32/Debug)
	find_library(OBJUTIL_LIB_D NAMES objUtil PATHS ${CMAKE_SOURCE_DIR}/ktx/lib/win32/Debug)
	
	find_library(KTX_LIB_R NAMES ktx PATHS ${CMAKE_SOURCE_DIR}/ktx/lib/win32/Release)
	find_library(KTX_READ_LIB_R NAMES ktx_read PATHS ${CMAKE_SOURCE_DIR}/ktx/lib/win32/Release)
	find_library(OBJUTIL_LIB_R NAMES objUtil PATHS ${CMAKE_SOURCE_DIR}/ktx/lib/win32/Release)
	
	target_link_libraries(LevelRenderer debug ${KTX_LIB_D} optimized ${KTX_LIB_R})
	target_link_libraries(LevelRenderer debug ${KTX_READ_LIB_D} optimized ${KTX_READ_LIB_R})
	target_link_libraries(LevelRenderer debug ${OBJUTIL_LIB_D} optimized ${OBJUTIL_LIB_R})
endif(WIN32)
