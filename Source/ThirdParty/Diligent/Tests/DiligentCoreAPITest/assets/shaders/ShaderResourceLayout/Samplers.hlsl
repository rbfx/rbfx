Texture2D g_Tex2D;

SamplerState g_Sam_Static;
SamplerState g_Sam_Mut;
SamplerState g_Sam_Dyn;

SamplerState g_SamArr_Static[STATIC_SAM_ARRAY_SIZE];  // 2
SamplerState g_SamArr_Mut   [MUTABLE_SAM_ARRAY_SIZE]; // 4
SamplerState g_SamArr_Dyn   [DYNAMIC_SAM_ARRAY_SIZE]; // 3

#define TexRefValue float4(1.0, 0.0, 1.0, 0.0)

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

    float2 UV = float2(0.5, 0.5);

    AllCorrect *= CheckValue(g_Tex2D.SampleLevel(g_Sam_Static, UV.xy, 0.0), TexRefValue);
    AllCorrect *= CheckValue(g_Tex2D.SampleLevel(g_Sam_Mut, UV.xy, 0.0),    TexRefValue);
    AllCorrect *= CheckValue(g_Tex2D.SampleLevel(g_Sam_Dyn, UV.xy, 0.0),    TexRefValue);

    // glslang is not smart enough to unroll the loops even when explicitly told to do so

    AllCorrect *= CheckValue(g_Tex2D.SampleLevel(g_SamArr_Static[0], UV.xy, 0.0), TexRefValue);
    AllCorrect *= CheckValue(g_Tex2D.SampleLevel(g_SamArr_Static[1], UV.xy, 0.0), TexRefValue);

    AllCorrect *= CheckValue(g_Tex2D.SampleLevel(g_SamArr_Mut[0], UV.xy, 0.0), TexRefValue);
    AllCorrect *= CheckValue(g_Tex2D.SampleLevel(g_SamArr_Mut[1], UV.xy, 0.0), TexRefValue);
    AllCorrect *= CheckValue(g_Tex2D.SampleLevel(g_SamArr_Mut[2], UV.xy, 0.0), TexRefValue);
    AllCorrect *= CheckValue(g_Tex2D.SampleLevel(g_SamArr_Mut[3], UV.xy, 0.0), TexRefValue);

    AllCorrect *= CheckValue(g_Tex2D.SampleLevel(g_SamArr_Dyn[0], UV.xy, 0.0), TexRefValue);
    AllCorrect *= CheckValue(g_Tex2D.SampleLevel(g_SamArr_Dyn[1], UV.xy, 0.0), TexRefValue);
    AllCorrect *= CheckValue(g_Tex2D.SampleLevel(g_SamArr_Dyn[2], UV.xy, 0.0), TexRefValue);

    return AllCorrect;
}

void VSMain(in  uint    VertId    : SV_VertexID,
            out float4 f4Color    : COLOR,
            out float4 f4Position : SV_Position)
{
    float4 Pos[6];
    Pos[0] = float4(-1.0, -0.5, 0.0, 1.0);
    Pos[1] = float4(-0.5, +0.5, 0.0, 1.0);
    Pos[2] = float4( 0.0, -0.5, 0.0, 1.0);

    Pos[3] = float4(+0.0, -0.5, 0.0, 1.0);
    Pos[4] = float4(+0.5, +0.5, 0.0, 1.0);
    Pos[5] = float4(+1.0, -0.5, 0.0, 1.0);

    f4Color = float4(VertId % 3 == 0 ? 1.0 : 0.0,
                     VertId % 3 == 1 ? 1.0 : 0.0,
                     VertId % 3 == 2 ? 1.0 : 0.0,
                     1.0) * VerifyResources();

    f4Position = Pos[VertId];
}

float4 PSMain(in float4 in_f4Color : COLOR,
              in float4 f4Position : SV_Position) : SV_Target
{
    return in_f4Color * VerifyResources();
}
