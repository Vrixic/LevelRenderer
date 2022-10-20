#pragma pack_matrix(row_major)

#define MAX_SUBMESH_PER_DRAW 512
#define MAX_LIGHTS_PER_DRAW 16

struct Material
{
    float3 Diffuse;
    float Dissolve; // Transparency
    float3 SpecularColor;
    float SpecularExponent;
    float3 Ambient;
    float Sharpness;
    float3 TransmissionFilter;
    float OpticalDensity;
    float3 Emissive;
    uint IlluminationModel;
};

struct PointLight
{
	/* W component is used for strength of the point light */
    float4 Position;

	/* W component is used for attenuation */
    float4 Color;
};

struct SpotLight
{
    /* W Component - spot light strength */
    float4 Position;

	/* W Component - cone ratio */
    float4 Color;
    
    /* W component - outer cone ratio*/
    float4 ConeDirection;
};

struct SceneDataGlobal
{
	/* Globally shared model information */
<<<<<<< HEAD
    float4x4 View[3];
=======
    float4x4 View[2];
>>>>>>> a376bcaca055c08eaa3440b4c38d28bc8fba93cc
    float4x4 Projection;

	/* Lighting Information */
    float4 LightDirection;
    float4 LightColor;
    float4 SunAmbient;
    float4 CameraWorldPosition;
    
    /* Per sub-mesh transform and material data */
    float4x4 Matrices[MAX_SUBMESH_PER_DRAW]; // World space matrices
    Material Materials[MAX_SUBMESH_PER_DRAW]; // color/texture of surface info of all meshes
    
    PointLight PointLights[MAX_LIGHTS_PER_DRAW];

    SpotLight SpotLights[MAX_LIGHTS_PER_DRAW];

	/* 16-byte padding for the lights,
	* X component -> num of point lights
	* Y component -> num of spot lights
	*/
    float4 NumOfLights;
};

/* Declare and access a Vulkan storage buffer in hlsl */
[[vk::binding(0)]]
StructuredBuffer<SceneDataGlobal> SceneData;

[[vk::push_constant]]
cbuffer ConstantBuffer
{
    uint MeshID;
    uint MaterialID;
    uint DiffuseTextureID;
    uint ViewMatID;
<<<<<<< HEAD
    
    float3 Color;
=======
>>>>>>> a376bcaca055c08eaa3440b4c38d28bc8fba93cc
};

struct VertexIn
{
    [[vk::location(0)]] float2 UV : TEXTCOORD0;
    [[vk::location(1)]] float3 Position : POSITION;
    [[vk::location(2)]] float3 Normal : NORMAL0;
    [[vk::location(3)]] float4 Color : COLOR0;
    
    [[vk::location(4)]] uint InstanceID : SV_INSTANCEID;
};

struct VertexOut
{
    float4 Position : SV_POSITION; // Homogeneous projection space
    float4 Color : COLOR0;
    float3 PositionWorld : WORLD; // position in world space
    float3 Normal : NORMAL0; // normal in world space
    float2 UV : TEXCOORD0;
};

VertexOut main(VertexIn inputVertex)
{
    VertexOut output;
	
    output.Position = float4(inputVertex.Position, 1);
    
    output.Position = mul(output.Position, SceneData[0].Matrices[MeshID + inputVertex.InstanceID]);
    output.PositionWorld = output.Position;
    output.Position = mul(output.Position, SceneData[0].View[ViewMatID]);
    output.Position = mul(output.Position, SceneData[0].Projection);
    
    output.Color = inputVertex.Color;
    output.Normal = mul(float4(inputVertex.Normal, 0.0f), SceneData[0].Matrices[MeshID + inputVertex.InstanceID]).xyz;
    output.UV = inputVertex.UV;
    
    return output;
}