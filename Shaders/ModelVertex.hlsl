#pragma pack_matrix(row_major)

#define MAX_SUBMESH_PER_DRAW 1024

struct OBJ_ATTRIBUTES
{
    float3 Diffuse; // diffuse reflectivity
    float Dissolve; // dissolve (transparency) 
    float3 Specular; // specular reflectivity
    float SpecularExponent; // specular exponent
    float3 Ambient; // ambient reflectivity
    float Sharpness; // local reflection map sharpness
    float3 TransmissionFilter; // transmission filter
    float OpticalDensity; // optical density (index of refraction)
    float3 Emissive; // emissive reflectivity
    uint IlluminationModel; // illumination model
};

struct GlobalModelData
{
	/* Globally shared model information */
    float4x4 View;
    float4x4 Projection;

	/* Lighting Information */
    float4 LightDirection;
    float4 LightColor;
    float4 SunAmbient;
    float4 CameraWorldPosition;
};

struct LocalModelData
{
    /* Per sub-mesh transform and material data */
    float4x4 Matrices[MAX_SUBMESH_PER_DRAW]; // World space matrices
    OBJ_ATTRIBUTES Materials[MAX_SUBMESH_PER_DRAW]; // color/texture of surface info of all meshes
};

/* Declare and access a Vulkan storage buffer in hlsl */
[[vk::binding(0)]]
StructuredBuffer<GlobalModelData> GlobalSceneData;

[[vk::binding(1)]]
StructuredBuffer<LocalModelData> LocalSceneData;

struct VertexIn
{
    [[vk::location(0)]] float2 UV : TEXTCOORD0;
    [[vk::location(1)]] float3 Position : POSITION;
    [[vk::location(2)]] float3 Normal : NORMAL0;
    [[vk::location(3)]] float4 Color : COLOR0;
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
    //output.Position = mul(float4(inputVertex.Position, 1.0f), SceneData[0].Matrices[MeshID]);
    //output.PositionWorld = output.Position;
    //output.Position = mul(output.Position, SceneData[0].View);
    //output.Position = mul(output.Position, SceneData[0].Projection);
    //
    //output.Normal = mul(float4(inputVertex.Normal, 0.0f), SceneData[0].Matrices[MeshID]).xyz;
    //output.UV = float2(0.0f, 0.0f);
	
    output.Position = float4(inputVertex.Position, 1);
    
    //output.Position = mul(output.Position, LocalSceneData[0].Matrices[0]);
    output.Position = mul(output.Position, GlobalSceneData[0].View);
    output.Position = mul(output.Position, GlobalSceneData[0].Projection);
    
    output.Color = inputVertex.Color;
    output.Normal = inputVertex.Normal;
    output.UV = inputVertex.UV;

	//return float4(inputVertex.Position, 1.0f);
    return output;
}