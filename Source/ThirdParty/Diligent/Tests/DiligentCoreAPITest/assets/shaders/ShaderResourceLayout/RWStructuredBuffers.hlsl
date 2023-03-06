struct BufferData
{
    float4 data;
};

RWStructuredBuffer<BufferData> g_RWBuff_Static;
RWStructuredBuffer<BufferData> g_RWBuff_Mut;
RWStructuredBuffer<BufferData> g_RWBuff_Dyn;

RWStructuredBuffer<BufferData> g_RWBuffArr_Static[STATIC_BUFF_ARRAY_SIZE];  // 4 or 1 in D3D11
RWStructuredBuffer<BufferData> g_RWBuffArr_Mut   [MUTABLE_BUFF_ARRAY_SIZE]; // 3 or 2 in D3D11
RWStructuredBuffer<BufferData> g_RWBuffArr_Dyn   [DYNAMIC_BUFF_ARRAY_SIZE]; // 2

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
    AllCorrect *= CheckValue(g_RWBuff_Static[1].data, Buff_Static_Ref);
    AllCorrect *= CheckValue(g_RWBuff_Mut   [2].data, Buff_Mut_Ref);
    AllCorrect *= CheckValue(g_RWBuff_Dyn   [3].data, Buff_Dyn_Ref);

    // Write to 0-th element
    float4 f4Data = float4(1.0, 2.0, 3.0, 4.0);
    g_RWBuff_Static[0].data = f4Data;
    g_RWBuff_Mut   [0].data = f4Data;
    g_RWBuff_Dyn   [0].data = f4Data;

    // glslang is not smart enough to unroll the loops even when explicitly told to do so

    AllCorrect *= CheckValue(g_RWBuffArr_Static[0][1].data, BuffArr_Static_Ref0);

    g_RWBuffArr_Static[0][0].data = f4Data;
#if (STATIC_BUFF_ARRAY_SIZE == 4)
    AllCorrect *= CheckValue(g_RWBuffArr_Static[1][1].data, BuffArr_Static_Ref1);
    AllCorrect *= CheckValue(g_RWBuffArr_Static[2][2].data, BuffArr_Static_Ref2);
    AllCorrect *= CheckValue(g_RWBuffArr_Static[3][3].data, BuffArr_Static_Ref3);

    g_RWBuffArr_Static[1][0].data = f4Data;
    g_RWBuffArr_Static[2][0].data = f4Data;
    g_RWBuffArr_Static[3][0].data = f4Data;
#endif

    AllCorrect *= CheckValue(g_RWBuffArr_Mut[0][1].data, BuffArr_Mut_Ref0);

    g_RWBuffArr_Mut[0][0].data = f4Data;
#if (MUTABLE_BUFF_ARRAY_SIZE == 3)
    AllCorrect *= CheckValue(g_RWBuffArr_Mut[1][2].data, BuffArr_Mut_Ref1);
    AllCorrect *= CheckValue(g_RWBuffArr_Mut[2][1].data, BuffArr_Mut_Ref2);

    g_RWBuffArr_Mut[1][0].data = f4Data;
    g_RWBuffArr_Mut[2][0].data = f4Data;
#endif

    AllCorrect *= CheckValue(g_RWBuffArr_Dyn[0][1].data, BuffArr_Dyn_Ref0);
    AllCorrect *= CheckValue(g_RWBuffArr_Dyn[1][2].data, BuffArr_Dyn_Ref1);

    g_RWBuffArr_Dyn[0][0].data = f4Data;
    g_RWBuffArr_Dyn[1][0].data = f4Data;

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
