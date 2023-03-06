#include "Defines.h"

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

struct ReloadTestData
{
    float4 VertColors[3];
    float4 RefTexColors[4];
};