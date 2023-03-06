Texture2D g_Tex2D_Static;
Texture2D g_Tex2D_Mut;
Texture2D g_Tex2D_Dyn;

SamplerState g_Tex2D_Static_sampler;
SamplerState g_Tex2D_Mut_sampler;
SamplerState g_Tex2D_Dyn_sampler;

cbuffer UniformBuff_Stat
{
    float4 g_Data_Stat;
}

cbuffer UniformBuff_Mut
{
    float4 g_Data_Mut;
}

cbuffer UniformBuff_Dyn
{
    float4 g_Data_Dyn;
}

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
    AllCorrect *= CheckValue(g_Tex2D_Static.SampleLevel(g_Tex2D_Static_sampler, UV.xy, 0.0), Tex2D_Static_Ref);
    AllCorrect *= CheckValue(g_Tex2D_Mut.   SampleLevel(g_Tex2D_Mut_sampler,    UV.xy, 0.0), Tex2D_Mut_Ref);
    AllCorrect *= CheckValue(g_Tex2D_Dyn.   SampleLevel(g_Tex2D_Dyn_sampler,    UV.xy, 0.0), Tex2D_Dyn_Ref);

    AllCorrect *= CheckValue(g_Data_Stat, Buff_Static_Ref);
    AllCorrect *= CheckValue(g_Data_Mut,  Buff_Mut_Ref);
    AllCorrect *= CheckValue(g_Data_Dyn,  Buff_Dyn_Ref);

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
