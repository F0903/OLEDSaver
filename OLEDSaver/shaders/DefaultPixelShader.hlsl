#include <Common.hlsli>

cbuffer DefaultConstantBuffer : register(b0)
{
    float time;
    float2 resolution;
}

cbuffer DefaultEffectConstantBuffer : register(b1)
{
    float effectTime;
    float duration;
}

float Ease(float x)
{
    return sin((x * 3.1415) * 0.5f) * 0.5f;
}

float CalcAlpha(float2 uv, float t)
{
    float stepA = step(t, uv.y);
    float stepB = step(uv.y, 1 - t);
    float stepTotal = stepA * stepB;
    return t * stepTotal + max(0, (1 - stepTotal));
}

float4 main(V2P input) : SV_TARGET
{
    float2 uv = input.position.xy / resolution.xy;
    
    float clampedTime = clamp(effectTime, 0, 1);
    float easedTime = Ease(clampedTime);
    
    float3 tophalf = step(easedTime, uv.y);
    float3 bottomhalf = step(1 - easedTime, uv.y);
    
    float4 total = float4(tophalf - bottomhalf, CalcAlpha(uv, easedTime));
    
    return total;
}