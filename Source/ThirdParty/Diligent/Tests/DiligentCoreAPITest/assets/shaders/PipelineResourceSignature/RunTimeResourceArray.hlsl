
#if TEXTURES_NONUNIFORM_INDEXING
#    define TEXTURES_NONUNIFORM(x) NonUniformResourceIndex(x)
#    define TEXTURES_COUNT         // unsized array
#else
#    define TEXTURES_NONUNIFORM(x) x
#    define TEXTURES_COUNT         NUM_TEXTURES
#endif

#if CONST_BUFFERS_NONUNIFORM_INDEXING
#    define CONST_BUFFERS_NONUNIFORM(x) NonUniformResourceIndex(x)
#    define CONST_BUFFERS_COUNT         // unsized array
#else
#    define CONST_BUFFERS_NONUNIFORM(x) x
#    define CONST_BUFFERS_COUNT         NUM_CONST_BUFFERS
#endif

#if FMT_BUFFERS_NONUNIFORM_INDEXING
#    define FMT_BUFFERS_NONUNIFORM(x) NonUniformResourceIndex(x)
#    define FMT_BUFFERS_COUNT         // unsized array
#else
#    define FMT_BUFFERS_NONUNIFORM(x) x
#    define FMT_BUFFERS_COUNT         NUM_FMT_BUFFERS
#endif

#if STRUCT_BUFFERS_NONUNIFORM_INDEXING
#    define STRUCT_BUFFERS_NONUNIFORM(x) NonUniformResourceIndex(x)
#    define STRUCT_BUFFERS_COUNT         // unsized array
#else
#    define STRUCT_BUFFERS_NONUNIFORM(x) x
#    define STRUCT_BUFFERS_COUNT         NUM_STRUCT_BUFFERS
#endif

#if RWTEXTURES_NONUNIFORM_INDEXING
#    define RWTEXTURES_NONUNIFORM(x) NonUniformResourceIndex(x)
#    define RWTEXTURES_COUNT         // unsized array
#else
#    define RWTEXTURES_NONUNIFORM(x) x
#    define RWTEXTURES_COUNT         NUM_RWTEXTURES
#endif

#if RWSTRUCT_BUFFERS_NONUNIFORM_INDEXING
#    define RWSTRUCT_BUFFERS_NONUNIFORM(x) NonUniformResourceIndex(x)
#    define RWSTRUCT_BUFFERS_COUNT         // unsized array
#else
#    define RWSTRUCT_BUFFERS_NONUNIFORM(x) x
#    define RWSTRUCT_BUFFERS_COUNT         NUM_RWSTRUCT_BUFFERS
#endif

#if RWFMT_BUFFERS_NONUNIFORM_INDEXING
#    define RWFMT_BUFFERS_NONUNIFORM(x) NonUniformResourceIndex(x)
#    define RWFMT_BUFFERS_COUNT         // unsized array
#else
#    define RWFMT_BUFFERS_NONUNIFORM(x) x
#    define RWFMT_BUFFERS_COUNT         NUM_RWFMT_BUFFERS
#endif

Texture2D g_Textures[TEXTURES_COUNT] : register(t0, space1);
SamplerState g_Samplers[] : register(s4, space27);

struct CBData
{
    float4 Data;
};
ConstantBuffer<CBData> g_ConstantBuffers[CONST_BUFFERS_COUNT] : register(b10, space5);

Buffer g_FormattedBuffers[FMT_BUFFERS_COUNT]: register(t15, space7);

struct StructBuffData
{
    float4 Data;
};
StructuredBuffer<StructBuffData> g_StructuredBuffers[STRUCT_BUFFERS_COUNT];

RWTexture2D<unorm float4 /*format=rgba8*/> g_RWTextures[RWTEXTURES_COUNT] : register(u10, space5);


#ifndef VULKAN // RW structured buffers are not supported by DXC
struct RWStructBuffData
{
    float4 Data;
};
RWStructuredBuffer<RWStructBuffData> g_RWStructBuffers[RWSTRUCT_BUFFERS_COUNT] : register(u10, space6);
#endif

RWBuffer<float4> g_RWFormattedBuffers[RWFMT_BUFFERS_COUNT] : register(u10, space41);

float4 CheckValue(float4 Val, float4 Expected)
{
    return float4(Val.x == Expected.x ? 1.0 : 0.0,
                  Val.y == Expected.y ? 1.0 : 0.0,
                  Val.z == Expected.z ? 1.0 : 0.0,
                  Val.w == Expected.w ? 1.0 : 0.0);
}


float4 VerifyResources(uint index, float2 coord)
{
    float4 TexRefValues[NUM_TEXTURES];
    TexRefValues[0] = Tex2D_Ref0;
    TexRefValues[1] = Tex2D_Ref1;
    TexRefValues[2] = Tex2D_Ref2;
    TexRefValues[3] = Tex2D_Ref3;
    TexRefValues[4] = Tex2D_Ref4;
    TexRefValues[5] = Tex2D_Ref5;
    TexRefValues[6] = Tex2D_Ref6;
    TexRefValues[7] = Tex2D_Ref7;

    float4 ConstBuffRefValues[NUM_CONST_BUFFERS];
    ConstBuffRefValues[0] = ConstBuff_Ref0;
    ConstBuffRefValues[1] = ConstBuff_Ref1;
    ConstBuffRefValues[2] = ConstBuff_Ref2;
    ConstBuffRefValues[3] = ConstBuff_Ref3;
    ConstBuffRefValues[4] = ConstBuff_Ref4;
    ConstBuffRefValues[5] = ConstBuff_Ref5;
    ConstBuffRefValues[6] = ConstBuff_Ref6;

    float4 FmtBuffRefValues[NUM_FMT_BUFFERS];
    FmtBuffRefValues[0] = FmtBuff_Ref0;
    FmtBuffRefValues[1] = FmtBuff_Ref1;
    FmtBuffRefValues[2] = FmtBuff_Ref2;
    FmtBuffRefValues[3] = FmtBuff_Ref3;
    FmtBuffRefValues[4] = FmtBuff_Ref4;

    float4 StructBuffRefValues[NUM_STRUCT_BUFFERS];
    StructBuffRefValues[0] = StructBuff_Ref0;
    StructBuffRefValues[1] = StructBuff_Ref1;
    StructBuffRefValues[2] = StructBuff_Ref2;

    float4 RWTexRefValues[NUM_RWTEXTURES];
    RWTexRefValues[0] = RWTex2D_Ref0;
    RWTexRefValues[1] = RWTex2D_Ref1;
    RWTexRefValues[2] = RWTex2D_Ref2;

    float4 RWStructBuffRefValues[NUM_RWSTRUCT_BUFFERS];
    RWStructBuffRefValues[0] = RWStructBuff_Ref0;
    RWStructBuffRefValues[1] = RWStructBuff_Ref1;
    RWStructBuffRefValues[2] = RWStructBuff_Ref2;
    RWStructBuffRefValues[3] = RWStructBuff_Ref3;

    float4 RWFmtBuffRefValues[NUM_RWFMT_BUFFERS];
    RWFmtBuffRefValues[0] = RWFmtBuff_Ref0;
    RWFmtBuffRefValues[1] = RWFmtBuff_Ref1;

    uint TexIdx          = index % NUM_TEXTURES;
    uint SamIdx          = index % NUM_SAMPLERS;
    uint ConstBuffIdx    = index % NUM_CONST_BUFFERS;
    uint FmtBuffIdx      = index % NUM_FMT_BUFFERS;
    uint StructBuffIdx   = index % NUM_STRUCT_BUFFERS;
    uint RWTexIdx        = index % NUM_RWTEXTURES;
    uint RWStructBuffIdx = index % NUM_RWSTRUCT_BUFFERS;
    uint RWFmtBuffIdx    = index % NUM_RWFMT_BUFFERS;

#ifdef USE_D3D12_WARP_BUG_WORKAROUND
    ConstBuffIdx = 0;
#endif

    float4 AllCorrect = float4(1.0, 1.0, 1.0, 1.0);
    AllCorrect *= CheckValue(g_Textures[TEXTURES_NONUNIFORM(TexIdx)].SampleLevel(g_Samplers[NonUniformResourceIndex(SamIdx)], coord, 0.0), TexRefValues[TexIdx]);
    AllCorrect *= CheckValue(g_ConstantBuffers[CONST_BUFFERS_NONUNIFORM(ConstBuffIdx)].Data, ConstBuffRefValues[ConstBuffIdx]);
    AllCorrect *= CheckValue(g_FormattedBuffers[FMT_BUFFERS_NONUNIFORM(FmtBuffIdx)].Load(0), FmtBuffRefValues[FmtBuffIdx]);
    AllCorrect *= CheckValue(g_StructuredBuffers[STRUCT_BUFFERS_NONUNIFORM(StructBuffIdx)][0].Data, StructBuffRefValues[StructBuffIdx]);
    AllCorrect *= CheckValue(g_RWTextures[RWTEXTURES_NONUNIFORM(RWTexIdx)][int2(coord*10)], RWTexRefValues[RWTexIdx]);
#ifndef VULKAN
    AllCorrect *= CheckValue(g_RWStructBuffers[RWSTRUCT_BUFFERS_NONUNIFORM(RWStructBuffIdx)][0].Data, RWStructBuffRefValues[RWStructBuffIdx]);
#endif
    AllCorrect *= CheckValue(g_RWFormattedBuffers[RWFMT_BUFFERS_NONUNIFORM(RWFmtBuffIdx)][0], RWFmtBuffRefValues[RWFmtBuffIdx]);

    return AllCorrect;
}

RWTexture2D<float4>  g_OutImage;

[numthreads(16, 16, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID,
          uint  LocalInvocationIndex : SV_GroupIndex)
{
    uint2 Dim;
    g_OutImage.GetDimensions(Dim.x, Dim.y);
    if (GlobalInvocationID.x >= Dim.x || GlobalInvocationID.y >= Dim.y)
        return;

    float4 Color = float4(float2(GlobalInvocationID.xy % 256u) / 256.0, 0.0, 1.0);
    float2 uv = float2(GlobalInvocationID.xy + float2(0.5,0.5)) / float2(Dim);
    Color *= VerifyResources(LocalInvocationIndex, uv);

    g_OutImage[GlobalInvocationID.xy] = Color;
}
