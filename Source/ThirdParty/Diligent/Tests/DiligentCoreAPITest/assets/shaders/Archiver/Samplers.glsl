
uniform sampler2D g_Tex2D_Static;
uniform sampler2D g_Tex2D_Mut;
uniform sampler2D g_Tex2DArr_Mut[MUTABLE_TEX_ARRAY_SIZE];
uniform sampler2D g_Tex2D_Dyn;
uniform sampler2D g_Tex2DArr_Dyn[DYNAMIC_TEX_ARRAY_SIZE];
uniform sampler2D g_Tex2DArr_Static[STATIC_TEX_ARRAY_SIZE];

uniform UniformBuff_Stat
{
    vec4 f4Data;
}g_Data_Stat;

uniform UniformBuff_Mut
{
    vec4 f4Data;
}g_Data_Mut;

uniform UniformBuff_Dyn
{
    vec4 f4Data;
}g_Data_Dyn;

vec4 CheckValue(vec4 Val, vec4 Expected)
{
    return vec4(Val.x == Expected.x ? 1.0 : 0.0,
                Val.y == Expected.y ? 1.0 : 0.0,
                Val.z == Expected.z ? 1.0 : 0.0,
                Val.w == Expected.w ? 1.0 : 0.0);
}

vec4 VerifyResources()
{
    vec4 AllCorrect = vec4(1.0, 1.0, 1.0, 1.0);

    AllCorrect *= CheckValue(g_Data_Stat.f4Data, Buff_Static_Ref);
    AllCorrect *= CheckValue(g_Data_Mut.f4Data,  Buff_Mut_Ref);
    AllCorrect *= CheckValue(g_Data_Dyn.f4Data,  Buff_Dyn_Ref);

    vec2 UV = vec2(0.5, 0.5);

    AllCorrect *= CheckValue(textureLod(g_Tex2D_Static, UV, 0.0), Tex2D_Static_Ref);
    AllCorrect *= CheckValue(textureLod(g_Tex2D_Mut,    UV, 0.0), Tex2D_Mut_Ref);
    AllCorrect *= CheckValue(textureLod(g_Tex2D_Dyn,    UV, 0.0), Tex2D_Dyn_Ref);

    AllCorrect *= CheckValue(textureLod(g_Tex2DArr_Static[0], UV, 0.0), Tex2DArr_Static_Ref0);
    AllCorrect *= CheckValue(textureLod(g_Tex2DArr_Static[1], UV, 0.0), Tex2DArr_Static_Ref1);

    AllCorrect *= CheckValue(textureLod(g_Tex2DArr_Mut[0], UV, 0.0), Tex2DArr_Mut_Ref0);
    AllCorrect *= CheckValue(textureLod(g_Tex2DArr_Mut[1], UV, 0.0), Tex2DArr_Mut_Ref1);
    AllCorrect *= CheckValue(textureLod(g_Tex2DArr_Mut[2], UV, 0.0), Tex2DArr_Mut_Ref2);
    AllCorrect *= CheckValue(textureLod(g_Tex2DArr_Mut[3], UV, 0.0), Tex2DArr_Mut_Ref3);

    AllCorrect *= CheckValue(textureLod(g_Tex2DArr_Dyn[0], UV, 0.0), Tex2DArr_Dyn_Ref0);
    AllCorrect *= CheckValue(textureLod(g_Tex2DArr_Dyn[1], UV, 0.0), Tex2DArr_Dyn_Ref1);
    AllCorrect *= CheckValue(textureLod(g_Tex2DArr_Dyn[2], UV, 0.0), Tex2DArr_Dyn_Ref2);

	return AllCorrect;
}


#ifdef VERTEX_SHADER

#ifndef GL_ES
out gl_PerVertex
{
    vec4 gl_Position;
};
#endif

layout(location = 0) out vec4 out_Color;

void main()
{
    vec4 Pos[6];
    Pos[0] = vec4(-1.0, -0.5, 0.0, 1.0);
    Pos[1] = vec4(-0.5, +0.5, 0.0, 1.0);
    Pos[2] = vec4(0.0, -0.5, 0.0, 1.0);

    Pos[3] = vec4(+0.0, -0.5, 0.0, 1.0);
    Pos[4] = vec4(+0.5, +0.5, 0.0, 1.0);
    Pos[5] = vec4(+1.0, -0.5, 0.0, 1.0);

    vec4 Col[3];
    Col[0] = vec4(1.0, 0.0, 0.0, 1.0);
    Col[1] = vec4(0.0, 1.0, 0.0, 1.0);
    Col[2] = vec4(0.0, 0.0, 1.0, 1.0);

#ifdef VULKAN
    gl_Position = Pos[gl_VertexIndex];
    out_Color   = Col[gl_VertexIndex % 3] * VerifyResources();
#else
    gl_Position = Pos[gl_VertexID];
    out_Color   = Col[gl_VertexID % 3] * VerifyResources();
#endif
}

#endif


#ifdef FRAGMENT_SHADER

layout(location = 0) in vec4 in_Color;
layout(location = 0) out vec4 out_Color;
void main()
{
    out_Color = in_Color * VerifyResources();
}

#endif
