//assets/shaders/Common.hlsl
//Same struct and constantbuffe

//VertexStruct
struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 tangent : TANGENT;
};

