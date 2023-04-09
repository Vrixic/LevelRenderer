#define MAX_SUBMESH_PER_DRAW 512
#define MAX_LIGHTS_PER_DRAW 16

#define TEXTURE_DIFFUSE_FLAG 0x00000002
#define TEXTURE_SPECULAR_FLAG 0x00000004
#define TEXTURE_NORMAL_FLAG 0x00000008

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

float CalcSpecularIntensity(float specularExponent, float3 lightDir, float3 viewDirection, float3 surfaceNormal)
{
    float3 HalfVector = normalize(reflect(lightDir, surfaceNormal));
    return max(pow(saturate(dot(viewDirection, HalfVector)), specularExponent), 0);
}

float4 CalcDirectionalLight(uint dirLightIndex, float3 surfaceNormal, float4 lightDirection)//, float specIntensity)
{
    float LightRatio = saturate(dot(-lightDirection.xyz, surfaceNormal));    
    return (LightRatio * lightDirection.w) * SceneData[0].DirectionalLights[dirLightIndex].Color;
}

float4 CalcPointLight(uint pointLightIndex, float3 pointLightDir, float3 distanceFromPixel, float3 surfaceNormal)
{    
    float LightRatio = saturate(dot(pointLightDir, surfaceNormal)) * SceneData[0].PointLights[pointLightIndex].Position.w;
    
    float Attenuation = 1.0f - saturate(length(distanceFromPixel) / SceneData[0].PointLights[pointLightIndex].Color.w);
    Attenuation *= Attenuation;
    
    float3 LightColor = LightRatio * SceneData[0].PointLights[pointLightIndex].Color.xyz;
    
    return Attenuation * float4(LightColor, 1);
}

float4 CalcSpotLight(uint spotLightIndex, float3 spotLightDir, float3 pixelPositionWorld, float3 surfaceNormal)
{    
    float SurfaceRatio = saturate(dot(-spotLightDir, SceneData[0].SpotLights[spotLightIndex].ConeDirection.xyz));
    float SpotFactor = (SurfaceRatio > SceneData[0].SpotLights[spotLightIndex].Color.w) ? 1 : 0;

    float LightRatio = saturate(dot(spotLightDir, surfaceNormal)) * SceneData[0].SpotLights[spotLightIndex].Position.w;
    
    float InnerConeRatio = SceneData[0].SpotLights[spotLightIndex].Color.w;
    float OuterConeRatio = SceneData[0].SpotLights[spotLightIndex].ConeDirection.w;
    
    // Attenuation calculation
    float Attenuation = 1.0f - saturate((InnerConeRatio - SurfaceRatio) / (InnerConeRatio - OuterConeRatio));
    Attenuation *= Attenuation;
    
    float3 LightColor = SpotFactor * LightRatio * SceneData[0].SpotLights[spotLightIndex].Color.xyz;
    
    return Attenuation * float4(LightColor, 1);
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
    
    if (SceneData[0].Materials[MaterialID].TextureFlags & TEXTURE_NORMAL_FLAG)
    {
        SurfaceNormal = PreturbNormal(SurfaceNormal, ViewDirection, input.UV);
    }
    //return float4(SurfaceNormal, 1.0f);    
    
   /* Specular */
    float SpecularExponent = (SceneData[0].Materials[MaterialID].SpecularExponent != 0) ? SceneData[0].Materials[MaterialID].SpecularExponent : 96;
    
    float4 SpecularLight = float4(0, 0, 0, 1);
    float4 DirectLight = float4(0, 0, 0, 1);
    
    for (uint i = 0; i < (uint) SceneData[0].NumOfLights.x; i++)
    {
        float4 LightDirection = SceneData[0].DirectionalLights[i].Direction;
        
        /* Specular Component */
        float SpecIntensity = CalcSpecularIntensity(SpecularExponent, LightDirection.xyz, ViewDirection, SurfaceNormal);
        if (SceneData[0].Materials[MaterialID].TextureFlags & TEXTURE_SPECULAR_FLAG)
        {
            SpecIntensity *= TextureMaps[SpecularTextureID].Sample(Sampler[SpecularTextureID], input.UV);
        }    
    
        SpecularLight += float4(SceneData[0].Materials[MaterialID].SpecularColor, 1.0f) * SpecIntensity;
        DirectLight += CalcDirectionalLight(i, SurfaceNormal, LightDirection);
    }
    
    for (uint i = 0; i < (uint) SceneData[0].NumOfLights.x; i++)
    {
        float3 DistanceFromPixel = SceneData[0].PointLights[i].Position.xyz - input.PositionWorld;
        float3 PointLightDir = normalize(DistanceFromPixel);
       
        /* Specular Component */
        float SpecIntensity = CalcSpecularIntensity(SpecularExponent, PointLightDir, ViewDirection, SurfaceNormal);
        if (SceneData[0].Materials[MaterialID].TextureFlags & TEXTURE_SPECULAR_FLAG)
        {
            SpecIntensity *= TextureMaps[SpecularTextureID].Sample(Sampler[SpecularTextureID], input.UV);
        }
        float4 LightResult = CalcPointLight(i, PointLightDir, DistanceFromPixel, SurfaceNormal);
        DirectLight += LightResult;
        SpecularLight += float4(SceneData[0].Materials[MaterialID].SpecularColor, 1.0f) * SpecIntensity * LightResult.w;
    }
    
    for (uint i = 0; i < (uint) SceneData[0].NumOfLights.y; i++)
    {
        float3 DistanceFromPixel = SceneData[0].SpotLights[i].Position.xyz - input.PositionWorld;
        float3 SpotLightDir = normalize(DistanceFromPixel);
        
         /* Specular Component */
        float SpecIntensity = CalcSpecularIntensity(SpecularExponent, SpotLightDir, ViewDirection, SurfaceNormal);
        if (SceneData[0].Materials[MaterialID].TextureFlags & TEXTURE_SPECULAR_FLAG)
        {
            SpecIntensity *= TextureMaps[SpecularTextureID].Sample(Sampler[SpecularTextureID], input.UV);
        }
        
        float4 LightResult = CalcSpotLight(i, SpotLightDir, input.PositionWorld, SurfaceNormal);
        DirectLight += LightResult;
        
        SpecularLight += float4(SceneData[0].Materials[MaterialID].SpecularColor, 1.0f) * SpecIntensity * LightResult.w;
    }
    
    return saturate(DirectLight + SceneData[0].SunAmbient) * DiffuseColor + SpecularLight;
}