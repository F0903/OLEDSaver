#include <Common.hlsli>

V2P main(float4 pos : POSITION)
{
    V2P output;
    output.position = pos;
    return output;
}