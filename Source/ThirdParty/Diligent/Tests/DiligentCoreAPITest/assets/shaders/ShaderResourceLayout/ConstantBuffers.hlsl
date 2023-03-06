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

#if ARRAYS_SUPPORTED
struct CBDataStat
{
    float4 Data;
};
struct CBDataMut
{
    float4 Data;
};
struct CBDataDyn
{
    float4 Data;
};
ConstantBuffer<CBDataStat> UniformBuffArr_Stat[STATIC_CB_ARRAY_SIZE];  // 2
ConstantBuffer<CBDataMut>  UniformBuffArr_Mut [MUTABLE_CB_ARRAY_SIZE]; // 4
ConstantBuffer<CBDataDyn>  UniformBuffArr_Dyn [DYNAMIC_CB_ARRAY_SIZE]; // 3
#endif

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

    AllCorrect *= CheckValue(g_Data_Stat, Buff_Static_Ref);
    AllCorrect *= CheckValue(g_Data_Mut,  Buff_Mut_Ref);
    AllCorrect *= CheckValue(g_Data_Dyn,  Buff_Dyn_Ref);

    // glslang is not smart enough to unroll the loops even when explicitly told to do so
#if ARRAYS_SUPPORTED
    AllCorrect *= CheckValue(UniformBuffArr_Stat[0].Data, BuffArr_Static_Ref0);
    AllCorrect *= CheckValue(UniformBuffArr_Stat[1].Data, BuffArr_Static_Ref1);

    AllCorrect *= CheckValue(UniformBuffArr_Mut[0].Data, BuffArr_Mut_Ref0);
#if MUTABLE_CB_ARRAY_SIZE == 4
    AllCorrect *= CheckValue(UniformBuffArr_Mut[1].Data, BuffArr_Mut_Ref1);
    AllCorrect *= CheckValue(UniformBuffArr_Mut[2].Data, BuffArr_Mut_Ref2);
    AllCorrect *= CheckValue(UniformBuffArr_Mut[3].Data, BuffArr_Mut_Ref3);
#endif

    AllCorrect *= CheckValue(UniformBuffArr_Dyn[0].Data, BuffArr_Dyn_Ref0);
#if DYNAMIC_CB_ARRAY_SIZE == 3
    AllCorrect *= CheckValue(UniformBuffArr_Dyn[1].Data, BuffArr_Dyn_Ref1);
    AllCorrect *= CheckValue(UniformBuffArr_Dyn[2].Data, BuffArr_Dyn_Ref2);
#endif

#endif

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
