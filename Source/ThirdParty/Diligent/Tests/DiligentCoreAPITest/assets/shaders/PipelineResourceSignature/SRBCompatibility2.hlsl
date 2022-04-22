cbuffer Constants
{
    float4 g_Data;
};

Texture2D    g_Texture;
SamplerState g_Texture_sampler;

Texture2D    g_Texture2;
SamplerState g_Texture2_sampler;

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

    float2 UV = float2(2.5, 3.5);

    AllCorrect *= CheckValue(g_Texture.SampleLevel(g_Texture_sampler, UV.xy, 0.0), Tex2D_Ref);
    AllCorrect *= CheckValue(g_Texture2.SampleLevel(g_Texture2_sampler, UV.xy, 0.0), Tex2D_2_Ref);
    AllCorrect *= CheckValue(g_Data, Buff_Ref);

    return AllCorrect;
}

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR;
};

void VSMain(in  uint     VertId : SV_VertexID,
            out VSOutput PSIn)
{
    float4 Pos[3];
    Pos[0] = float4(+0.0, -0.5, 0.0, 1.0);
    Pos[1] = float4(+0.5, +0.5, 0.0, 1.0);
    Pos[2] = float4(+1.0, -0.5, 0.0, 1.0);

    PSIn.Color = float4(VertId % 3 == 0 ? 1.0 : 0.0,
                        VertId % 3 == 1 ? 1.0 : 0.0,
                        VertId % 3 == 2 ? 1.0 : 0.0,
                        1.0) * VerifyResources();

    PSIn.Position = Pos[VertId];
}

float4 PSMain(in VSOutput PSIn) : SV_Target
{
    return PSIn.Color * VerifyResources();
}
