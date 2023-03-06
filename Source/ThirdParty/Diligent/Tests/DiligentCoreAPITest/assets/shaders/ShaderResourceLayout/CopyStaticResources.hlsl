Texture2D g_Tex2D_0;
Texture2D g_Tex2D_1;
Texture2D g_Tex2D_2;
Texture2D g_Tex2D_3;

SamplerState g_Tex2D_0_sampler;
SamplerState g_Tex2D_1_sampler;
SamplerState g_Tex2D_2_sampler;
SamplerState g_Tex2D_3_sampler;

cbuffer UniformBuff_0
{
    float4 g_Data0;
}

cbuffer UniformBuff_1
{
    float4 g_Data1;
}

cbuffer UniformBuff_2
{
    float4 g_Data2;
}

Buffer g_FmtBuff_0;
Buffer g_FmtBuff_1;
Buffer g_FmtBuff_2;

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
    AllCorrect *= CheckValue(g_Tex2D_0.SampleLevel(g_Tex2D_0_sampler, UV.xy, 0.0), Tex2D_0_Ref);
    AllCorrect *= CheckValue(g_Tex2D_1.SampleLevel(g_Tex2D_1_sampler, UV.xy, 0.0), Tex2D_1_Ref);
    AllCorrect *= CheckValue(g_Tex2D_2.SampleLevel(g_Tex2D_2_sampler, UV.xy, 0.0), Tex2D_2_Ref);
    AllCorrect *= CheckValue(g_Tex2D_3.SampleLevel(g_Tex2D_3_sampler, UV.xy, 0.0), Tex2D_3_Ref);

    AllCorrect *= CheckValue(g_Data0, UniformBuff_0_Ref);
    AllCorrect *= CheckValue(g_Data1, UniformBuff_1_Ref);
    AllCorrect *= CheckValue(g_Data2, UniformBuff_2_Ref);

    AllCorrect *= CheckValue(g_FmtBuff_0.Load(0), FmtBuff_0_Ref);
    AllCorrect *= CheckValue(g_FmtBuff_1.Load(0), FmtBuff_1_Ref);
    AllCorrect *= CheckValue(g_FmtBuff_2.Load(0), FmtBuff_2_Ref);

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

float4 PSMain(in float4 f4Color    : COLOR, // Name must match VS output
              in float4 f4Position : SV_Position) : SV_Target
{
    return f4Color * VerifyResources();
}
