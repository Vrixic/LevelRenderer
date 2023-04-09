#define NUM_CONTROL_POINTS 3

struct VertexOut
{
    float4 Position : SV_POSITION; // Homogeneous projection space
    float4 Color : COLOR0;
    float3 PositionWorld : WORLD; // position in world space
    float3 Normal : NORMAL0; // normal in world space
    float2 UV : TEXCOORD0;
};

struct HullOutput
{
    float4 Position : SV_POSITION; // Homogeneous projection space
    float4 Color : COLOR0;
    float3 PositionWorld : WORLD; // position in world space
    float3 Normal : NORMAL0; // normal in world space
    float2 UV : TEXCOORD0;
};

struct HullShaderConstantDataOutput
{
    // [3] edges on a triangle, would be [4] for a quad domain
    float EdgeTessFactor[3] : SV_TessFactor;
    
    // would be InsideTessFactor[2] for a quad domain
    float InsideTessFactor : SV_InsideTessFactor;
};

// Patch Constant function
HullShaderConstantDataOutput ClacHSPatchConstants(
    InputPatch<VertexOut, NUM_CONTROL_POINTS> inputPatch,
    uint patchID : SV_PrimitiveID)
{
    HullShaderConstantDataOutput Output;
    
    // Set interior & edge tessellation factors
    Output.EdgeTessFactor[0] = Output.EdgeTessFactor[1] = Output.EdgeTessFactor[2] = 8; // ~8 partitions
    return Output;
}

[domain("tri")] // tessellator builds triangles 
[partitioning("fractional_odd")] // type of division applied
[outputtopology("triangle_cw")] // topology of output
[outputcontrolpoints(NUM_CONTROL_POINTS)] // num control points sent to Domain Shader
// points to the HLSL function which computes tessellation amount
[patchconstantfunc("ClacHSPatchConstants")]
HullOutput main(
    InputPatch<VertexOut, NUM_CONTROL_POINTS> inputPatch, 
    uint i : SV_OutputControlPointID, uint patchID : SV_PrimitiveID)
{
    HullOutput Output;
    // Direct transfer of control points to domain shader
    Output.Position = inputPatch[i].Position;
    Output.Color = inputPatch[i].Color;
    Output.PositionWorld = inputPatch[i].PositionWorld;
    Output.Normal = inputPatch[i].Normal;
    Output.UV = inputPatch[i].UV;
    
    return Output;
}
