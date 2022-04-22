Texture2D g_Tex2DClamp;
Texture2D g_Tex2DWrap;
Texture2D g_Tex2DMirror;

SamplerState g_Tex2DClamp_sampler;
SamplerState g_Tex2DWrap_sampler;
SamplerState g_Tex2DMirror_sampler;

float4 CheckValue(float4 Val, float4 Expected)
{
    return float4(Val.x == Expected.x ? 1.0 : 0.0,
                  Val.y == Expected.y ? 1.0 : 0.0,
                  Val.z == Expected.z ? 1.0 : 0.0,
                  Val.w == Expected.w ? 1.0 : 0.0);
}

float4 GetRefClampValue(float2 UV, float4 Colors[2][2])
{
    UV = saturate(UV);
    int2 ij = min(int2(UV * 2.0), int2(1,1));
    return Colors[ij.x][ij.y];
}

float4 GetRefWrapValue(float2 UV, float4 Colors[2][2])
{
    UV = frac(UV);
    int2 ij = int2(UV * 2.0);
    return Colors[ij.x][ij.y];
}

float4 GetRefMirrorValue(float2 UV, float4 Colors[2][2])
{
    UV = frac(UV * 0.5) * 2.0;
    if (UV.x > 1.0) UV.x = 2.0 - UV.x;
    if (UV.y > 1.0) UV.y = 2.0 - UV.y;
    int2 ij = int2(UV * 2.0);
    return Colors[ij.x][ij.y];
}

float4 VerifyResources()
{
    float4 Colors[2][2];
    Colors[0][0] = float4(1.0, 0.0, 0.0, 0.0);
    Colors[1][0] = float4(0.0, 1.0, 0.0, 0.0);
    Colors[0][1] = float4(0.0, 0.0, 1.0, 0.0);
    Colors[1][1] = float4(0.0, 0.0, 0.0, 1.0);

    float4 AllCorrect = float4(1.0, 1.0, 1.0, 1.0);

    for(int x = 0; x < 10; ++x)
    {
        for(int y = 0; y < 10; ++y)
        {
            float2 UV = float2(x, y) * 0.5 + float2(0.25, 0.25);
            AllCorrect *= CheckValue(g_Tex2DClamp.SampleLevel(g_Tex2DClamp_sampler, UV.xy, 0.0), GetRefClampValue(UV, Colors));
            AllCorrect *= CheckValue(g_Tex2DWrap.SampleLevel(g_Tex2DWrap_sampler, UV.xy, 0.0), GetRefWrapValue(UV, Colors));
            AllCorrect *= CheckValue(g_Tex2DMirror.SampleLevel(g_Tex2DMirror_sampler, UV.xy, 0.0), GetRefMirrorValue(UV, Colors));
        }
    }

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

float4 PSMain(in float4 f4Color : COLOR, // Name must match VS output
              in float4 f4Position : SV_Position) : SV_Target
{
    return f4Color * VerifyResources();
}
