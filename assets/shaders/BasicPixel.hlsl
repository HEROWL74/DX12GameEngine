// shaders/BasicPixel.hlsl
struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLD_POSITION;
    float3 color : COLOR;
};

//Material ConstantBuffer
cbuffer MaterialConstants : register(b2)
{
    float4 albedo; // xyz: albedo, w: metallic
    float4 roughnessAoEmissiveStrength; // x: roughness, y: ao, z: emissiveStrength, w: padding
    float4 emissive; // xyz: emissive, w: normalStrength
    float4 alphaParams; // x: alpha, y: useAlphaTest, z: alphaTestThreshold, w: heightScale
    float4 uvTransform; // xy: uvScale, zw: uvOffset
};

//Texture&Sampler
//Texture2D albedoTexture : register(t0);
//SamplerState linearSampler : register(s0);

float4 main(VertexOutput input) : SV_TARGET
{
    //create UV
    float2 uv = input.worldPos.xy * uvTransform.zw;
    
    
    //Material's Albedo + VertexColor
    float3 materialColor = albedo.rgb;
    float3 vertexColor = input.color;
    
    //Color++
    float3 finalColor = materialColor;
    
    float alpha = alphaParams.x;
    
    if(alphaParams.y > 0.5)
    {
        if(alpha < alphaParams.z)
        {
            discard;
        }
    }
    
    return float4(finalColor, alpha);
}