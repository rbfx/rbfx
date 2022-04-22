#version 460 core
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_nonuniform_qualifier : require

#if TEXTURES_NONUNIFORM_INDEXING
#    define TEXTURES_NONUNIFORM(x) nonuniformEXT(x)
#    define TEXTURES_COUNT         // unsized array
#else
#    define TEXTURES_NONUNIFORM(x) x
#    define TEXTURES_COUNT         NUM_TEXTURES
#endif

#if CONST_BUFFERS_NONUNIFORM_INDEXING
#    define CONST_BUFFERS_NONUNIFORM(x) nonuniformEXT(x)
#    define CONST_BUFFERS_COUNT         // unsized array
#else
#    define CONST_BUFFERS_NONUNIFORM(x) x
#    define CONST_BUFFERS_COUNT         NUM_CONST_BUFFERS
#endif

#if FMT_BUFFERS_NONUNIFORM_INDEXING
#    define FMT_BUFFERS_NONUNIFORM(x) nonuniformEXT(x)
#    define FMT_BUFFERS_COUNT         // unsized array
#else
#    define FMT_BUFFERS_NONUNIFORM(x) x
#    define FMT_BUFFERS_COUNT         NUM_FMT_BUFFERS
#endif

#if STRUCT_BUFFERS_NONUNIFORM_INDEXING
#    define STRUCT_BUFFERS_NONUNIFORM(x) nonuniformEXT(x)
#    define STRUCT_BUFFERS_COUNT         // unsized array
#else
#    define STRUCT_BUFFERS_NONUNIFORM(x) x
#    define STRUCT_BUFFERS_COUNT         NUM_STRUCT_BUFFERS
#endif

#if RWTEXTURES_NONUNIFORM_INDEXING
#    define RWTEXTURES_NONUNIFORM(x) nonuniformEXT(x)
#    define RWTEXTURES_COUNT         // unsized array
#else
#    define RWTEXTURES_NONUNIFORM(x) x
#    define RWTEXTURES_COUNT         NUM_RWTEXTURES
#endif

#if RWSTRUCT_BUFFERS_NONUNIFORM_INDEXING
#    define RWSTRUCT_BUFFERS_NONUNIFORM(x) nonuniformEXT(x)
#    define RWSTRUCT_BUFFERS_COUNT         // unsized array
#else
#    define RWSTRUCT_BUFFERS_NONUNIFORM(x) x
#    define RWSTRUCT_BUFFERS_COUNT         NUM_RWSTRUCT_BUFFERS
#endif

#if RWFMT_BUFFERS_NONUNIFORM_INDEXING
#    define RWFMT_BUFFERS_NONUNIFORM(x) nonuniformEXT(x)
#    define RWFMT_BUFFERS_COUNT         // unsized array
#else
#    define RWFMT_BUFFERS_NONUNIFORM(x) x
#    define RWFMT_BUFFERS_COUNT         NUM_RWFMT_BUFFERS
#endif

uniform texture2D g_Textures[TEXTURES_COUNT];
uniform sampler g_Samplers[];

uniform g_ConstantBuffers
{
    vec4 Data;
}g_ConstantBufferInst[CONST_BUFFERS_COUNT];

uniform samplerBuffer g_FormattedBuffers[FMT_BUFFERS_COUNT];

layout(std140) readonly buffer g_StructuredBuffers
{
    vec4 Data;
}g_StructuredBufferInst[STRUCT_BUFFERS_COUNT];

layout(rgba8) uniform image2D g_RWTextures[RWTEXTURES_COUNT];

layout(std140) buffer g_RWStructBuffers
{
    vec4 Data;
}g_RWStructBuffersInst[RWSTRUCT_BUFFERS_COUNT];


layout(rgba32f) uniform imageBuffer g_RWFormattedBuffers[RWFMT_BUFFERS_COUNT];


vec4 CheckValue(vec4 Val, vec4 Expected)
{
    return vec4(Val.x == Expected.x ? 1.0 : 0.0,
                Val.y == Expected.y ? 1.0 : 0.0,
                Val.z == Expected.z ? 1.0 : 0.0,
                Val.w == Expected.w ? 1.0 : 0.0);
}


vec4 VerifyResources(uint index, vec2 coord)
{
    vec4 TexRefValues[NUM_TEXTURES];
    TexRefValues[0] = Tex2D_Ref0;
    TexRefValues[1] = Tex2D_Ref1;
    TexRefValues[2] = Tex2D_Ref2;
    TexRefValues[3] = Tex2D_Ref3;
    TexRefValues[4] = Tex2D_Ref4;
    TexRefValues[5] = Tex2D_Ref5;
    TexRefValues[6] = Tex2D_Ref6;
    TexRefValues[7] = Tex2D_Ref7;

    vec4 ConstBuffRefValues[NUM_CONST_BUFFERS];
    ConstBuffRefValues[0] = ConstBuff_Ref0;
    ConstBuffRefValues[1] = ConstBuff_Ref1;
    ConstBuffRefValues[2] = ConstBuff_Ref2;
    ConstBuffRefValues[3] = ConstBuff_Ref3;
    ConstBuffRefValues[4] = ConstBuff_Ref4;
    ConstBuffRefValues[5] = ConstBuff_Ref5;
    ConstBuffRefValues[6] = ConstBuff_Ref6;

    vec4 FmtBuffRefValues[NUM_FMT_BUFFERS];
    FmtBuffRefValues[0] = FmtBuff_Ref0;
    FmtBuffRefValues[1] = FmtBuff_Ref1;
    FmtBuffRefValues[2] = FmtBuff_Ref2;
    FmtBuffRefValues[3] = FmtBuff_Ref3;
    FmtBuffRefValues[4] = FmtBuff_Ref4;

    vec4 StructBuffRefValues[NUM_STRUCT_BUFFERS];
    StructBuffRefValues[0] = StructBuff_Ref0;
    StructBuffRefValues[1] = StructBuff_Ref1;
    StructBuffRefValues[2] = StructBuff_Ref2;

    vec4 RWTexRefValues[NUM_RWTEXTURES];
    RWTexRefValues[0] = RWTex2D_Ref0;
    RWTexRefValues[1] = RWTex2D_Ref1;
    RWTexRefValues[2] = RWTex2D_Ref2;

    vec4 RWStructBuffRefValues[NUM_RWSTRUCT_BUFFERS];
    RWStructBuffRefValues[0] = RWStructBuff_Ref0;
    RWStructBuffRefValues[1] = RWStructBuff_Ref1;
    RWStructBuffRefValues[2] = RWStructBuff_Ref2;
    RWStructBuffRefValues[3] = RWStructBuff_Ref3;

    vec4 RWFmtBuffRefValues[NUM_RWFMT_BUFFERS];
    RWFmtBuffRefValues[0] = RWFmtBuff_Ref0;
    RWFmtBuffRefValues[1] = RWFmtBuff_Ref1;

    uint TexIdx          = index % NUM_TEXTURES;
    uint SamIdx          = index % NUM_SAMPLERS;
    uint BuffIdx         = index % NUM_CONST_BUFFERS;
    uint FmtBuffIdx      = index % NUM_FMT_BUFFERS;
    uint StructBuffIdx   = index % NUM_STRUCT_BUFFERS;
    uint RWTexIdx        = index % NUM_RWTEXTURES;
    uint RWStructBuffIdx = index % NUM_RWSTRUCT_BUFFERS;
    uint RWFmtBuffIdx    = index % NUM_RWFMT_BUFFERS;

    vec4 AllCorrect = vec4(1.0, 1.0, 1.0, 1.0);
    AllCorrect *= CheckValue(textureLod(sampler2D(g_Textures[TEXTURES_NONUNIFORM(TexIdx)], g_Samplers[nonuniformEXT(SamIdx)]), coord, 0.0), TexRefValues[TexIdx]);
    AllCorrect *= CheckValue(g_ConstantBufferInst[CONST_BUFFERS_NONUNIFORM(BuffIdx)].Data, ConstBuffRefValues[BuffIdx]);
    AllCorrect *= CheckValue(texelFetch(g_FormattedBuffers[FMT_BUFFERS_NONUNIFORM(FmtBuffIdx)], 0), FmtBuffRefValues[FmtBuffIdx]);
    AllCorrect *= CheckValue(g_StructuredBufferInst[STRUCT_BUFFERS_NONUNIFORM(StructBuffIdx)].Data, StructBuffRefValues[StructBuffIdx]);
    AllCorrect *= CheckValue(imageLoad(g_RWTextures[RWTEXTURES_NONUNIFORM(RWTexIdx)], ivec2(coord * 10)), RWTexRefValues[RWTexIdx]);
    AllCorrect *= CheckValue(g_RWStructBuffersInst[RWSTRUCT_BUFFERS_NONUNIFORM(RWStructBuffIdx)].Data, RWStructBuffRefValues[RWStructBuffIdx]);
    AllCorrect *= CheckValue(imageLoad(g_RWFormattedBuffers[RWFMT_BUFFERS_NONUNIFORM(RWFmtBuffIdx)], 0), RWFmtBuffRefValues[RWFmtBuffIdx]);

    return AllCorrect;
}

layout(rgba8) writeonly uniform image2D  g_OutImage;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    ivec2 Dim = imageSize(g_OutImage);
    if (gl_GlobalInvocationID.x >= uint(Dim.x) || gl_GlobalInvocationID.y >= uint(Dim.y))
        return;

    vec4 Color = vec4(vec2(gl_GlobalInvocationID.xy % 256u) / 256.0, 0.0, 1.0);
    vec2 uv = vec2(gl_GlobalInvocationID.xy + vec2(0.5,0.5)) / vec2(Dim);
    Color *= VerifyResources(gl_LocalInvocationIndex, uv);

    imageStore(g_OutImage, ivec2(gl_GlobalInvocationID.xy),  Color);
}
