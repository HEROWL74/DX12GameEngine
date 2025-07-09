// shaders/BasicPixel.hlsl
struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

float4 main(VertexOutput input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}