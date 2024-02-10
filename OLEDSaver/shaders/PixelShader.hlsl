#include <Common.hlsli>

cbuffer ConstantBuffer : register(b0)
{
    float time;
    float2 resolution;
}

float easeInOutQuart(float x)
{
    return 0 < 0.5 ? 4 * x * x * x * x : 1 - pow(-2 * x + 2, 8) / 2;
}

float calcAlpha(float2 uv, float t)
{
    return uv.y > t && uv.y < 1 - t ? t : 1.0f;
}

float4 main(V2P input) : SV_TARGET
{
    float2 uv = input.position.xy / resolution.xy;
    
    float clampedTime = easeInOutQuart(time * 0.65f);
    
    float3 tophalf = step(clampedTime, uv.y);
    float3 bottomhalf = step(1 - clampedTime, uv.y);
    
    float4 total = float4(tophalf - bottomhalf, calcAlpha(uv, clampedTime));
    
    return total;
}