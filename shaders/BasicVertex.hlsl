// shaders/BasicVertex.hlsl
cbuffer CameraConstants : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 viewProjectionMatrix;
    float3 cameraPosition;
    float padding;
};

cbuffer ObjectConstants : register(b1)
{
    float4x4 worldMatrix;
    float4x4 worldViewProjectionMatrix;
    float3 objectPosition;
    float padding2;
};

struct VertexInput
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    
    // 簡単な方法: 事前計算された worldViewProjectionMatrix を使用
    output.position = mul(float4(input.position, 1.0f), worldViewProjectionMatrix);
    
    // 色をそのまま渡す
    output.color = input.color;
    
    return output;
}