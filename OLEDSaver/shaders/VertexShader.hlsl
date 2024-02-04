struct VertexInfo
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct V2P
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

V2P main( float4 pos : POSITION, float4 rgba : COLOR )
{
    V2P output;
    output.position = pos;
    output.color = rgba;
    return output;
}