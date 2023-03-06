RWTexture2D<unorm float4 /*format=rgba8*/> g_RWTex2D_0;
RWTexture2D<unorm float4 /*format=rgba8*/> g_RWTex2D_1;
RWTexture2D<unorm float4 /*format=rgba8*/> g_RWTex2D_2;

struct BufferData
{
    float4 data;
};

RWStructuredBuffer<BufferData> g_RWBuff_0;
RWStructuredBuffer<BufferData> g_RWBuff_1;
RWStructuredBuffer<BufferData> g_RWBuff_2;

float4 CheckValue(float4 Val, float4 Expected)
{
    return float4(Val.x == Expected.x ? 1.0 : 0.0,
                  Val.y == Expected.y ? 1.0 : 0.0,
                  Val.z == Expected.z ? 1.0 : 0.0,
                  Val.w == Expected.w ? 1.0 : 0.0);
}

float4 VerifyResources()
{
    float4 AllCorrect = float4(1.0, 1.0, 1.0, 1.0);

    // Read from elements 1,2,3
    AllCorrect *= CheckValue(g_RWBuff_0[1].data, Buff_0_Ref);
    AllCorrect *= CheckValue(g_RWBuff_1[2].data, Buff_1_Ref);
    AllCorrect *= CheckValue(g_RWBuff_2[3].data, Buff_2_Ref);

    // Write to 0-th element
    float4 f4Data = float4(1.0, 2.0, 3.0, 4.0);
    g_RWBuff_0[0].data = f4Data;
    g_RWBuff_1[0].data = f4Data;
    g_RWBuff_2[0].data = f4Data;

    AllCorrect *= CheckValue(g_RWTex2D_0[int2(10, 12)], RWTex2D_0_Ref);
    AllCorrect *= CheckValue(g_RWTex2D_1[int2(14, 17)], RWTex2D_1_Ref);
    AllCorrect *= CheckValue(g_RWTex2D_2[int2(31, 24)], RWTex2D_2_Ref);
        
    float4 f4Color = float4(1.0, 2.0, 3.0, 4.0);
    g_RWTex2D_0[int2(0,0)] = f4Color;
    g_RWTex2D_1[int2(0,0)] = f4Color;
    g_RWTex2D_2[int2(0,0)] = f4Color;

    return AllCorrect;
}

RWTexture2D</*format=rgba8*/ float4> g_tex2DUAV;

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint2 ui2Dim;
    g_tex2DUAV.GetDimensions(ui2Dim.x, ui2Dim.y);
    if (DTid.x >= ui2Dim.x || DTid.y >= ui2Dim.y)
        return;

    g_tex2DUAV[DTid.xy] = float4(float2(DTid.xy % 256u) / 256.0, 0.0, 1.0) * VerifyResources();
}
