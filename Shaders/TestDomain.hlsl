#pragma pack_matrix(row_major)

#define NUM_CONTROL_POINTS 3
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
    uint TextureFlags;
    //float OpticalDensity;
    float3 Emissive;
    uint IlluminationModel;
};

struct DirectionalLight
{
    float4 Direction;
    float4 Color;
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
    float4 ConeDirection;
};

struct SceneDataGlobal
{
	/* Globally shared model information */
    float4x4 View[3];
    float4x4 Projection;

	/* Lighting Information */    
    float4 SunAmbient;
    float4 CameraWorldPosition;
    
    /* Per sub-mesh transform and material data */
    float4x4 Matrices[MAX_SUBMESH_PER_DRAW]; // World space matrices
    Material Materials[MAX_SUBMESH_PER_DRAW]; // color/texture of surface info of all meshes    
    
    DirectionalLight DirectionalLights[MAX_LIGHTS_PER_DRAW];
    
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

    float3 Color;
    uint SpecularTextureID;
    uint NormalTextureID;
};

struct HullShaderConstantDataOutput
{
    // [3] edges on a triangle, would be [4] for a quad domain
    float EdgeTessFactor[3] : SV_TessFactor;
    
    // would be InsideTessFactor[2] for a quad domain
    float InsideTessFactor : SV_InsideTessFactor;
};

struct HullOutput
{
    float4 Position : SV_POSITION; // Homogeneous projection space
    float4 Color : COLOR0;
    float3 PositionWorld : WORLD; // position in world space
    float3 Normal : NORMAL0; // normal in world space
    float2 UV : TEXCOORD0;
};


struct DomainOutput
{
    // Projected position to rasterizer
    float4 Position : SV_POSITION; // Homogeneous projection space
    float4 Color : COLOR0;
    float3 PositionWorld : WORLD; // position in world space
    float3 Normal : NORMAL0; // normal in world space
    float2 UV : TEXCOORD0;
};

[domain("tri")]
DomainOutput main(
    HullShaderConstantDataOutput input, float3 domain : SV_DomainLocation,
    const OutputPatch<HullOutput, NUM_CONTROL_POINTS> patch, uint InstanceID : SV_INSTANCEID)
{
    DomainOutput Output;
    // domain location used to manually interpolate newly created geometry across overall triangle
    Output.Position = patch[0].Position * domain.x + patch[1].Position * domain.y + patch[2].Position * domain.z;
    Output.Color = patch[0].Color * domain.x + patch[1].Color * domain.y + patch[2].Color * domain.z;
    Output.Normal = patch[0].Normal * domain.x + patch[1].Normal * domain.y + patch[2].Normal * domain.z;
    Output.UV = patch[0].UV * domain.x + patch[1].UV * domain.y + patch[2].UV * domain.z;
    
    Output.Position = mul(Output.Position, SceneData[0].Matrices[MeshID + InstanceID]);
    Output.PositionWorld = Output.Position;
    Output.Position = mul(Output.Position, SceneData[0].View[ViewMatID]);
    Output.Position = mul(Output.Position, SceneData[0].Projection);
    
    Output.Normal = mul(float4(Output.Normal, 0.0f), SceneData[0].Matrices[MeshID + InstanceID]).xyz;
    
    return Output;
}
