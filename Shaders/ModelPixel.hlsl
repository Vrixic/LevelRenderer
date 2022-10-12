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

struct PixelIn
{
    float4 Position : SV_POSITION; // Homogeneous projection space
    float4 Color : COLOR0;
    float3 PositionWorld : WORLD; // position in world space
    float3 Normal : NORMAL0; // normal in world space
    float2 UV : TEXCOORD0;
};

// an ultra simple hlsl pixel shader
// TODO: Part 4b
float4 main(PixelIn input) : SV_TARGET
{
    //float4 DiffuseColor = float4(SceneData[0].Materials[MeshID].Diffuse, 1.0f);
    //float3 SurfaceNormal = normalize(input.Normal);
    //float3 LightDirection = SceneData[0].LightDirection.xyz;
    //float LightRatio = saturate(dot(SurfaceNormal, -LightDirection));
    //
    ///* Specular Component */
    //float3 ViewDirection = normalize(SceneData[0].CameraWorldPosition.xyz - input.PositionWorld);
    //float3 HalfVector = normalize(reflect(LightDirection, SurfaceNormal));
    //float Intensity = max(pow(saturate(dot(ViewDirection, HalfVector)), SceneData[0].Materials[MeshID].SpecularExponent), 0);
    //
    //float4 ReflectedLight = float4(SceneData[0].Materials[MeshID].Specular, 1.0f) * 1.0f * Intensity;
    //
    //return saturate(LightRatio * SceneData[0].LightColor + SceneData[0].SunAmbient) * DiffuseColor + ReflectedLight;
	
    return input.Color * GlobalSceneData[0].SunAmbient;
}