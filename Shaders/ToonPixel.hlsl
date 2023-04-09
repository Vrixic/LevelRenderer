#define MAX_SUBMESH_PER_DRAW 512
#define MAX_LIGHTS_PER_DRAW 16

#define TEXTURE_DIFFUSE_FLAG 0x00000001
#define TEXTURE_SPECULAR_FLAG 0x00000002
#define TEXTURE_NORMAL_FLAG 0x00000003

[[vk::binding(0, 1)]]
Texture2D TextureMaps[] : register(t1);
[[vk::binding(0, 1)]]
SamplerState Sampler[] : register(s1);

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
    
    /* W component - outer cone ratio*/
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
    
    float3 FresnelColor; // 12 bytes
    uint NormalTextureID;

    uint Padding[20];
};

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
   
     // Calculate the real position of this pixel in 3d space, taking into account
    // the rotation and scale of the model. It's a useful formula for some effects.
    // This could also be done in the vertex shader

    // Calculate the normal including the model rotation and scale
    if (SceneData[0].NumOfLights.z < 0)
    {
        return SceneData[0].SunAmbient;
    }
    float3 LightVector = normalize(SceneData[0].DirectionalLights[0].Direction.xyx - input.PositionWorld);

    // An example simple lighting effect, taking the dot product of the normal
    // (which way this pixel is pointing) and a user generated light position
    float3 SurfaceNormal = normalize(input.Normal);
    float brightness = dot(SurfaceNormal, LightVector);

    float4 Final;
    float3 Color = float3(0.0f, 1.0f, 1.0f);
    if (brightness > 0.75)
        Final = float4(Color * 0.75f, 1.0);
    else if (brightness > 0.5)
        Final = float4(Color * 0.5f, 1.0);
    else if (brightness > 0.25)
        Final = float4(Color * 0.25, 1.0);
    else
        Final = float4(Color * 0.01, 1.0);

    // Fragment shaders set the gl_FragColor, which is a vector4 of
    // ( red, green, blue, alpha ).
    return Final;

}