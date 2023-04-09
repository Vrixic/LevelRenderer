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

float4 CalcDirectionalLight(float3 lightDir, float3 surfaceNormal, float3 viewDirection, float4 diffuseColor, float2 uv)//, float specIntensity)
{
    float LightRatio = saturate(dot(-lightDir, surfaceNormal));
    
    /* Specular Component */  
    float3 HalfVector = normalize(reflect(lightDir, surfaceNormal));
    float SpecIntensity = 1.0f;
    if (SceneData[0].Materials[MaterialID].TextureFlags & TEXTURE_SPECULAR_FLAG)
    {
        SpecIntensity = TextureMaps[SpecularTextureID].Sample(Sampler[SpecularTextureID], uv);
    }
    
    float SpecularExponent = (SceneData[0].Materials[MaterialID].SpecularExponent != 0) ? SceneData[0].Materials[MaterialID].SpecularExponent : 96;
    float Intensity = max(pow(saturate(dot(viewDirection, HalfVector)), SpecularExponent), 0);
        
    float4 ReflectedLight = float4(SceneData[0].Materials[MaterialID].SpecularColor, 1.0f) * 1.0f * Intensity * SpecIntensity;
    
    return (LightRatio * SceneData[0].LightDirection.w) * SceneData[0].LightColor * diffuseColor + ReflectedLight;
}

float4 CalcPointLight(uint pointLightIndex, float3 pixelPositionWorld, float3 surfaceNormal, float4 diffuseColor)
{
    float3 DistanceFromPixel = SceneData[0].PointLights[pointLightIndex].Position.xyz - pixelPositionWorld;
    float3 PointLightDir = normalize(DistanceFromPixel);
    
    float LightRatio = saturate(dot(PointLightDir, surfaceNormal)) * SceneData[0].PointLights[pointLightIndex].Position.w;
    
    float Attenuation = 1.0f - saturate(length(DistanceFromPixel) / SceneData[0].PointLights[pointLightIndex].Color.w);
    Attenuation *= Attenuation;
    
    return Attenuation * LightRatio * float4(SceneData[0].PointLights[pointLightIndex].Color.xyz * diffuseColor.xyz, 1);
}

float4 CalcSpotLight(uint spotLightIndex, float3 pixelPositionWorld, float3 surfaceNormal, float4 diffuseColor)
{
    float3 DistanceFromPixel = SceneData[0].SpotLights[spotLightIndex].Position.xyz - pixelPositionWorld;
    float3 SpotLightDir = normalize(DistanceFromPixel);
    
    float SurfaceRatio = saturate(dot(-SpotLightDir, SceneData[0].SpotLights[spotLightIndex].ConeDirection.xyz));
    float SpotFactor = (SurfaceRatio > SceneData[0].SpotLights[spotLightIndex].Color.w) ? 1 : 0;

    float LightRatio = saturate(dot(SpotLightDir, surfaceNormal)) * SceneData[0].SpotLights[spotLightIndex].Position.w;
    
    float InnerConeRatio = SceneData[0].SpotLights[spotLightIndex].Color.w;
    float OuterConeRatio = SceneData[0].SpotLights[spotLightIndex].ConeDirection.w;
    
    // Attenuation calculation
    float Attenuation = 1.0f - saturate((InnerConeRatio - SurfaceRatio) / (InnerConeRatio - OuterConeRatio));
    Attenuation *= Attenuation;
    
    return Attenuation * SpotFactor * LightRatio * float4(SceneData[0].SpotLights[spotLightIndex].Color.xyz * diffuseColor.xyz, 1);
}

float3x3 Inverse3x3(float3x3 mat)
{
    float3x3 Mat = transpose(mat);
    float Det = dot(cross(Mat[0], Mat[1]), Mat[2]);
    float3x3 Adjugate = float3x3(cross(Mat[1], Mat[2]), cross(Mat[2], Mat[0]), cross(Mat[0], Mat[1]));
    
    return Adjugate / Det;
}

float3x3 CotangentFrame(float3 normal, float3 viewDir, float2 uv)
{
    // Get the edge vectors of the pixel triangle
    float3 DP1 = ddx(viewDir);
    float3 DP2 = ddy(viewDir);
    
    float2 DUV1 = ddx(uv);
    float2 DUV2 = ddy(uv);

    // Solve the linear system
    float3 DP2Perp = cross(DP2, normal);
    float3 DP1Perp = cross(normal, DP1);
    
    float3 Tangent = DP2Perp * DUV1.x + DP1Perp * DUV2.x;
    float3 Bitangent = DP2Perp * DUV1.y + DP1Perp * DUV2.y;
    
    // Construct a scale-invariant frame
    float InvMax = rsqrt(max(dot(Tangent, Tangent), dot(Bitangent, Bitangent)));
    return float3x3(Tangent * InvMax, Bitangent * InvMax, normal);
}

float3 PreturbNormal(float3 normal, float3 v, float2 texCoord)
{
    // assume N, the interpolated vertex normal and V, the view vector (vertex to eye)
    float3 Map = normalize(TextureMaps[NormalTextureID].Sample(Sampler[NormalTextureID], texCoord).xyz * 2.0f - 1.0f);
    float3x3 TBN = CotangentFrame(normal, -v, texCoord);
    //Map.y = -Map.y;
    return normalize(mul(Map, TBN));
}

float CalcFresnel(float start, float end, float normalDotView)
{
    return saturate(smoothstep(start, end, (1.0f - normalDotView)));
}

// an ultra simple hlsl pixel shader
// TODO: Part 4b
float4 main(PixelIn input) : SV_TARGET
{
    float4 DiffuseColor = float4(SceneData[0].Materials[MaterialID].Diffuse, 1.0f);
    if (SceneData[0].Materials[MaterialID].TextureFlags & TEXTURE_DIFFUSE_FLAG)
    {
        DiffuseColor = TextureMaps[DiffuseTextureID].Sample(Sampler[DiffuseTextureID], input.UV);
    }
    
    float3 SurfaceNormal = normalize(input.Normal);
    float3 ViewDirection = normalize(SceneData[0].CameraWorldPosition.xyz - input.PositionWorld);
    
    float Fresnel = CalcFresnel(0.25f, 1, dot(SurfaceNormal, ViewDirection));
    
    if (SceneData[0].Materials[MaterialID].TextureFlags & TEXTURE_NORMAL_FLAG)
    {
        SurfaceNormal = PreturbNormal(SurfaceNormal, ViewDirection, input.UV);
    }
    //return float4(SurfaceNormal, 1.0f);    
    
    float3 LightDirection = SceneData[0].LightDirection.xyz;
    
    float4 Result = CalcDirectionalLight(LightDirection, SurfaceNormal, ViewDirection, DiffuseColor, input.UV); //, SpecIntensity);
    
    for (uint i = 0; i < (uint) SceneData[0].NumOfLights.x; i++)
    {
        Result += CalcPointLight(i, input.PositionWorld, SurfaceNormal, DiffuseColor);
    }
    
    for (uint i = 0; i < (uint) SceneData[0].NumOfLights.y; i++)
    {
        Result += CalcSpotLight(i, input.PositionWorld, SurfaceNormal, DiffuseColor);
    }
    
    return (saturate(Result * SceneData[0].SunAmbient) * (1 - Fresnel)) + float4(FresnelColor, 1) * Fresnel;
}