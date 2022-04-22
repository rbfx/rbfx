layout(std140) readonly buffer g_Buff_Static
{
    vec4 data;
}g_StorageBuff_Static;

layout(std140) readonly buffer g_Buff_Mut
{
    vec4 data;
}g_StorageBuff_Mut;

layout(std140) readonly buffer g_Buff_Dyn
{
    vec4 data;
}g_StorageBuff_Dyn;


#ifdef FRAGMENT_SHADER
// Vulkan only allows 16 dynamic storage buffer bindings among all stages, so
// use arrays only in fragment shader
layout(std140) readonly buffer g_BuffArr_Static
{
    vec4 data;
}g_StorageBuffArr_Static[STATIC_BUFF_ARRAY_SIZE];  // 4

layout(std140) readonly buffer g_BuffArr_Mut
{
    vec4 data;
}g_StorageBuffArr_Mut[MUTABLE_BUFF_ARRAY_SIZE]; // 3

layout(std140) readonly buffer g_BuffArr_Dyn
{
    vec4 data;
}g_StorageBuffArr_Dyn[DYNAMIC_BUFF_ARRAY_SIZE]; // 2
#endif

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

    AllCorrect *= CheckValue(g_StorageBuff_Static.data, Buff_Static_Ref);
    AllCorrect *= CheckValue(g_StorageBuff_Mut   .data, Buff_Mut_Ref);
    AllCorrect *= CheckValue(g_StorageBuff_Dyn   .data, Buff_Dyn_Ref);

    // glslang is not smart enough to unroll the loops even when explicitly told to do so
#ifdef FRAGMENT_SHADER
    AllCorrect *= CheckValue(g_StorageBuffArr_Static[0].data, BuffArr_Static_Ref0);
    AllCorrect *= CheckValue(g_StorageBuffArr_Static[1].data, BuffArr_Static_Ref1);
    AllCorrect *= CheckValue(g_StorageBuffArr_Static[2].data, BuffArr_Static_Ref2);
    AllCorrect *= CheckValue(g_StorageBuffArr_Static[3].data, BuffArr_Static_Ref3);

    AllCorrect *= CheckValue(g_StorageBuffArr_Mut[0].data, BuffArr_Mut_Ref0);
    AllCorrect *= CheckValue(g_StorageBuffArr_Mut[1].data, BuffArr_Mut_Ref1);
    AllCorrect *= CheckValue(g_StorageBuffArr_Mut[2].data, BuffArr_Mut_Ref2);

    AllCorrect *= CheckValue(g_StorageBuffArr_Dyn[0].data, BuffArr_Dyn_Ref0);
    AllCorrect *= CheckValue(g_StorageBuffArr_Dyn[1].data, BuffArr_Dyn_Ref1);
#endif

    return AllCorrect;
}

#ifdef VERTEX_SHADER

//To use any built-in input or output in the gl_PerVertex and
//gl_PerFragment blocks in separable program objects, shader code must
//redeclare those blocks prior to use.
//
// Declaring this block causes compilation error on NVidia GLES
#ifndef GL_ES
out gl_PerVertex
{
    vec4 gl_Position;
};
#endif

layout(location = 0)out vec4 out_Color;

void main()
{
    vec4 Pos[6];
    Pos[0] = vec4(-1.0, -0.5, 0.0, 1.0);
    Pos[1] = vec4(-0.5, +0.5, 0.0, 1.0);
    Pos[2] = vec4( 0.0, -0.5, 0.0, 1.0);

    Pos[3] = vec4(+0.0, -0.5, 0.0, 1.0);
    Pos[4] = vec4(+0.5, +0.5, 0.0, 1.0);
    Pos[5] = vec4(+1.0, -0.5, 0.0, 1.0);

    vec4 Col[3];
    Col[0] = vec4(1.0, 0.0, 0.0, 1.0);
    Col[1] = vec4(0.0, 1.0, 0.0, 1.0);
    Col[2] = vec4(0.0, 0.0, 1.0, 1.0);

#ifdef VULKAN
    gl_Position = Pos[gl_VertexIndex];
    out_Color = Col[gl_VertexIndex % 3] * VerifyResources();
#else
    gl_Position = Pos[gl_VertexID];
    out_Color = Col[gl_VertexID % 3] * VerifyResources();
#endif
}
#endif

#ifdef FRAGMENT_SHADER
layout(location = 0)in vec4 in_Color;
layout(location = 0)out vec4 out_Color;
void main()
{
    out_Color = in_Color * VerifyResources();
}
#endif
