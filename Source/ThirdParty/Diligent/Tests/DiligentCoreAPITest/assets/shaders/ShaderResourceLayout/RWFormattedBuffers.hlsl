RWBuffer<float4 /*format=rgba32f*/> g_RWBuff_Static;
RWBuffer<float4 /*format=rgba32f*/> g_RWBuff_Mut;
RWBuffer<float4 /*format=rgba32f*/> g_RWBuff_Dyn;

RWBuffer<float4 /*format=rgba32f*/> g_RWBuffArr_Static[STATIC_BUFF_ARRAY_SIZE];  // 4 or 1 in D3D11
RWBuffer<float4 /*format=rgba32f*/> g_RWBuffArr_Mut   [MUTABLE_BUFF_ARRAY_SIZE]; // 3 or 2 in D3D11
RWBuffer<float4 /*format=rgba32f*/> g_RWBuffArr_Dyn   [DYNAMIC_BUFF_ARRAY_SIZE]; // 2

float4 CheckValue(float4 Val, float4 Expected)
{
    return float4(Val.x == Expected.x ? 1.0 : 0.0,
                  Val.y == Expected.y ? 1.0 : 0.0,
                  Val.z == Expected.z ? 1.0 : 0.0,
                  Val.w == Expected.w ? 1.0 : 0.0);
}

#ifdef METAL
// Metal does not support read-write access to formatted buffers
#   define CHECK_VALUE(Val, Expected) float4(1.0, 1.0, 1.0, 1.0)
#else
#   define CHECK_VALUE CheckValue
#endif

float4 VerifyResources()
{
    float4 AllCorrect = float4(1.0, 1.0, 1.0, 1.0);

    // Read from elements 1,2,3
    AllCorrect *= CHECK_VALUE(g_RWBuff_Static[1], Buff_Static_Ref);
    AllCorrect *= CHECK_VALUE(g_RWBuff_Mut   [2], Buff_Mut_Ref);
    AllCorrect *= CHECK_VALUE(g_RWBuff_Dyn   [3], Buff_Dyn_Ref);

    // Write to element 0
    float4 f4Data = float4(1.0, 2.0, 3.0, 4.0);
    g_RWBuff_Static[0] = f4Data;
    g_RWBuff_Mut   [0] = f4Data;
    g_RWBuff_Dyn   [0] = f4Data;

    // glslang is not smart enough to unroll the loops even when explicitly told to do so

    AllCorrect *= CHECK_VALUE(g_RWBuffArr_Static[0][1], BuffArr_Static_Ref0);

    g_RWBuffArr_Static[0][0] = f4Data;
#if (STATIC_BUFF_ARRAY_SIZE == 4)
    AllCorrect *= CHECK_VALUE(g_RWBuffArr_Static[1][1], BuffArr_Static_Ref1);
    AllCorrect *= CHECK_VALUE(g_RWBuffArr_Static[2][2], BuffArr_Static_Ref2);
    AllCorrect *= CHECK_VALUE(g_RWBuffArr_Static[3][3], BuffArr_Static_Ref3);

    g_RWBuffArr_Static[1][0] = f4Data;
    g_RWBuffArr_Static[2][0] = f4Data;
    g_RWBuffArr_Static[3][0] = f4Data;
#endif

    AllCorrect *= CHECK_VALUE(g_RWBuffArr_Mut[0][1], BuffArr_Mut_Ref0);
    g_RWBuffArr_Mut[0][0] = f4Data;

#if (MUTABLE_BUFF_ARRAY_SIZE == 3)
    AllCorrect *= CHECK_VALUE(g_RWBuffArr_Mut[1][2], BuffArr_Mut_Ref1);
    AllCorrect *= CHECK_VALUE(g_RWBuffArr_Mut[2][2], BuffArr_Mut_Ref2);

    g_RWBuffArr_Mut[1][0] = f4Data;
    g_RWBuffArr_Mut[2][0] = f4Data;
#endif

    AllCorrect *= CHECK_VALUE(g_RWBuffArr_Dyn[0][1], BuffArr_Dyn_Ref0);
    AllCorrect *= CHECK_VALUE(g_RWBuffArr_Dyn[1][2], BuffArr_Dyn_Ref1);

    g_RWBuffArr_Dyn[0][0] = f4Data;
    g_RWBuffArr_Dyn[1][0] = f4Data;

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
