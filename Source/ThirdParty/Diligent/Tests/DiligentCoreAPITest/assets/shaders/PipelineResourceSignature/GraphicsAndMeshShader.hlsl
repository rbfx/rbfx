struct VSOutput
{
    float4 f4Position : SV_Position;
    float4 f4Color    : COLOR;
};

float4 CheckValue(float4 Val, float4 Expected)
{
    return float4(Val.x == Expected.x ? 1.0 : 0.0,
                  Val.y == Expected.y ? 1.0 : 0.0,
                  Val.z == Expected.z ? 1.0 : 0.0,
                  Val.w == Expected.w ? 1.0 : 0.0);
}


#if defined(VERTEX_SHADER) || defined(MESH_SHADER)
cbuffer Constants
{
    float4 g_Data;
};

float4 VerifyResourcesVS()
{
    float4 AllCorrect = float4(1.0, 1.0, 1.0, 1.0);

    AllCorrect *= CheckValue(g_Data, Buff_Ref);
	return AllCorrect;
}

void VSMain(in  uint     VertId : SV_VertexID,
            out VSOutput Out)
{
    float4 Pos[3];
    Pos[0] = float4(-1.0, -0.5, 0.0, 1.0);
    Pos[1] = float4(-0.5, +0.5, 0.0, 1.0);
    Pos[2] = float4( 0.0, -0.5, 0.0, 1.0);

    Out.f4Position = Pos[VertId];
    Out.f4Color = float4(VertId % 3 == 0 ? 1.0 : 0.0,
                         VertId % 3 == 1 ? 1.0 : 0.0,
                         VertId % 3 == 2 ? 1.0 : 0.0,
                         1.0) * VerifyResourcesVS();
}

[numthreads(3,1,1)]
[outputtopology("triangle")]
void MSMain(             uint     I      : SV_GroupIndex,
            out indices  uint3    tris[1],
            out vertices VSOutput verts[3])
{
    SetMeshOutputCounts(3, 1);

    if (I == 0)
		tris[0] = uint3(0, 1, 2);

    float4 Pos[3];
    Pos[0] = float4(+0.0, -0.5, 0.0, 1.0);
    Pos[1] = float4(+0.5, +0.5, 0.0, 1.0);
    Pos[2] = float4(+1.0, -0.5, 0.0, 1.0);

	verts[I].f4Position = Pos[I];
    verts[I].f4Color    = float4(I % 3 == 0 ? 1.0 : 0.0,
                                 I % 3 == 1 ? 1.0 : 0.0,
                                 I % 3 == 2 ? 1.0 : 0.0,
                                 1.0) * VerifyResourcesVS();
}
#endif


#ifdef PIXEL_SHADER
Texture2D    g_Texture;
SamplerState g_Texture_sampler;

float4 VerifyResourcesPS()
{
    float4 AllCorrect = float4(1.0, 1.0, 1.0, 1.0);
    float2 UV         = float2(2.5, 3.5);

    AllCorrect *= CheckValue(g_Texture.SampleLevel(g_Texture_sampler, UV.xy, 0.0), Tex2D_Ref);
	return AllCorrect;
}

float4 PSMain(in VSOutput In) : SV_Target
{
    return In.f4Color * VerifyResourcesPS();
}
#endif
