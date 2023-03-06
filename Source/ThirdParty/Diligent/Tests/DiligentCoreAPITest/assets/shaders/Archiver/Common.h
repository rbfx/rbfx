struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
    float2 UV    : TEXCOORD0;
};

#if TEST_MACRO == 1
cbuffer cbConstants
{
    float4 UVScale;
    float4 ColorScale;
    float4 NormalScale;
    float4 DepthScale;
}
#endif
