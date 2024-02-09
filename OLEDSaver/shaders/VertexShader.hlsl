#include <Common.hlsli>

struct VertexInfo
{
    float3 position : POSITION;
};

V2P main(float4 pos : POSITION)
{
    V2P output;
    output.position = pos;
    return output;
}