Buffer g_Buff_Static;
Buffer g_Buff_Mut;
Buffer g_Buff_Dyn;

Buffer g_BuffArr_Static[STATIC_BUFF_ARRAY_SIZE];  // 4
Buffer g_BuffArr_Mut   [MUTABLE_BUFF_ARRAY_SIZE]; // 3
Buffer g_BuffArr_Dyn   [DYNAMIC_BUFF_ARRAY_SIZE]; // 2

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

    AllCorrect *= CheckValue(g_Buff_Static.Load(0), Buff_Static_Ref);
    AllCorrect *= CheckValue(g_Buff_Mut.   Load(1), Buff_Mut_Ref);
    AllCorrect *= CheckValue(g_Buff_Dyn.   Load(2), Buff_Dyn_Ref);

    // glslang is not smart enough to unroll the loops even when explicitly told to do so

    AllCorrect *= CheckValue(g_BuffArr_Static[0].Load(0), BuffArr_Static_Ref0);
    AllCorrect *= CheckValue(g_BuffArr_Static[1].Load(1), BuffArr_Static_Ref1);
    AllCorrect *= CheckValue(g_BuffArr_Static[2].Load(2), BuffArr_Static_Ref2);
    AllCorrect *= CheckValue(g_BuffArr_Static[3].Load(3), BuffArr_Static_Ref3);

    AllCorrect *= CheckValue(g_BuffArr_Mut[0].Load(0), BuffArr_Mut_Ref0);
    AllCorrect *= CheckValue(g_BuffArr_Mut[1].Load(1), BuffArr_Mut_Ref1);
    AllCorrect *= CheckValue(g_BuffArr_Mut[2].Load(2), BuffArr_Mut_Ref2);

    AllCorrect *= CheckValue(g_BuffArr_Dyn[0].Load(0), BuffArr_Dyn_Ref0);
    AllCorrect *= CheckValue(g_BuffArr_Dyn[1].Load(1), BuffArr_Dyn_Ref1);

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
