#include <Common.hlsli>

cbuffer ConstantBuffer : register(b0)
{
    float time;
    float2 resolution;
}

sampler2D screenshot;

float4 main(V2P input) : SV_TARGET
{
    float2 uv = input.position.xy / resolution.xy;
    float3 color = 0.5 + 0.5 * cos(time + uv.xyx + float3(0, 2, 4));
    
    return float4(color, 1.0);
}