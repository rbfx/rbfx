RWTexture2D<unorm float4 /*format=rgba8*/> g_RWTex2D_Static;
RWTexture2D<unorm float4 /*format=rgba8*/> g_RWTex2D_Mut;
RWTexture2D<unorm float4 /*format=rgba8*/> g_RWTex2D_Dyn;

RWTexture2D<unorm float4 /*format=rgba8*/> g_RWTex2DArr_Static[STATIC_TEX_ARRAY_SIZE];  // 2
RWTexture2D<unorm float4 /*format=rgba8*/> g_RWTex2DArr_Mut   [MUTABLE_TEX_ARRAY_SIZE]; // 4 or 2 in D3D11
RWTexture2D<unorm float4 /*format=rgba8*/> g_RWTex2DArr_Dyn   [DYNAMIC_TEX_ARRAY_SIZE]; // 3 or 1 in D3D11

float4 CheckValue(float4 Val, float4 Expected)
{
    return float4(Val.x == Expected.x ? 1.0 : 0.0,
                  Val.y == Expected.y ? 1.0 : 0.0,
                  Val.z == Expected.z ? 1.0 : 0.0,
                  Val.w == Expected.w ? 1.0 : 0.0);
}

#ifdef METAL
// Metal does not support read-write access to textures
#   define CHECK_VALUE(Val, Expected) float4(1.0, 1.0, 1.0, 1.0)
#else
#   define CHECK_VALUE CheckValue
#endif

float4 VerifyResources()
{
    float4 AllCorrect = float4(1.0, 1.0, 1.0, 1.0);

    AllCorrect *= CHECK_VALUE(g_RWTex2D_Static[int2(5, 6)], Tex2D_Static_Ref);
    AllCorrect *= CHECK_VALUE(g_RWTex2D_Mut   [int2(6, 7)], Tex2D_Mut_Ref);
    AllCorrect *= CHECK_VALUE(g_RWTex2D_Dyn   [int2(7, 8)], Tex2D_Dyn_Ref);

    float4 f4Color = float4(1.0, 2.0, 3.0, 4.0);
    g_RWTex2D_Static[int2(0,0)] = f4Color;
    g_RWTex2D_Mut   [int2(0,0)] = f4Color;
    g_RWTex2D_Dyn   [int2(0,0)] = f4Color;

    // glslang is not smart enough to unroll the loops even when explicitly told to do so

    AllCorrect *= CHECK_VALUE(g_RWTex2DArr_Static[0][int2(10, 12)], Tex2DArr_Static_Ref0);
    AllCorrect *= CHECK_VALUE(g_RWTex2DArr_Static[1][int2(14, 17)], Tex2DArr_Static_Ref1);

    g_RWTex2DArr_Static[0][int2(0,0)] = f4Color;
    g_RWTex2DArr_Static[1][int2(0,0)] = f4Color;

    AllCorrect *= CHECK_VALUE(g_RWTex2DArr_Mut[0][int2(32, 21)], Tex2DArr_Mut_Ref0);
    g_RWTex2DArr_Mut[0][int2(0,0)] = f4Color;

#if (MUTABLE_TEX_ARRAY_SIZE == 4)
    AllCorrect *= CHECK_VALUE(g_RWTex2DArr_Mut[1][int2(31, 24)], Tex2DArr_Mut_Ref1);
    AllCorrect *= CHECK_VALUE(g_RWTex2DArr_Mut[2][int2(42, 56)], Tex2DArr_Mut_Ref2);
    AllCorrect *= CHECK_VALUE(g_RWTex2DArr_Mut[3][int2(45, 54)], Tex2DArr_Mut_Ref3);

    g_RWTex2DArr_Mut[1][int2(0,0)] = f4Color;
    g_RWTex2DArr_Mut[2][int2(0,0)] = f4Color;
    g_RWTex2DArr_Mut[3][int2(0,0)] = f4Color;
#endif

    AllCorrect *= CHECK_VALUE(g_RWTex2DArr_Dyn[0][int2(67, 54)], Tex2DArr_Dyn_Ref0);
    g_RWTex2DArr_Dyn[0][int2(0,0)] = f4Color;

#if (DYNAMIC_TEX_ARRAY_SIZE == 3)
    AllCorrect *= CHECK_VALUE(g_RWTex2DArr_Dyn[1][int2(73, 58)], Tex2DArr_Dyn_Ref1);
    AllCorrect *= CHECK_VALUE(g_RWTex2DArr_Dyn[2][int2(78, 92)], Tex2DArr_Dyn_Ref2);

    g_RWTex2DArr_Dyn[1][int2(0,0)] = f4Color;
    g_RWTex2DArr_Dyn[2][int2(0,0)] = f4Color;
#endif

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
