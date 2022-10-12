#pragma pack_matrix( row_major )   
// an ultra simple hlsl vertex shader
// TODO: Part 2b
[[vk::push_constant]]
cbuffer ConstantBuffer
{
    matrix World;
    matrix View;
};
// TODO: Part 2f, Part 3b
// TODO: Part 1c
struct VSInput
{
    float4 Position : POSITION;
};
float4 main(VSInput inputVertex) : SV_POSITION
{
	// TODO: Part 2d
    float4 posWorld = mul(inputVertex.Position, World);
	// TODO: Part 2f, Part 3b
    float4 posView = mul(posWorld, View);

    return posView;
}