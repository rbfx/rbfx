layout(std140, binding = 0) buffer g_RWBuff_Static
{
    vec4 data[4];
}g_StorageBuff_Static;

layout(std140, binding = 1) buffer g_RWBuff_Mut
{
    vec4 data[4];
}g_StorageBuff_Mut;

layout(std140, binding = 2) buffer g_RWBuff_Dyn
{
    vec4 data[4];
}g_StorageBuff_Dyn;


layout(std140, binding = 3) buffer g_RWBuffArr_Static
{
    vec4 data[4];
}g_StorageBuffArr_Static[STATIC_BUFF_ARRAY_SIZE];  // 4 or 1 in OpenGL

layout(std140, binding = 7) buffer g_RWBuffArr_Mut
{
    vec4 data[4];
}g_StorageBuffArr_Mut[MUTABLE_BUFF_ARRAY_SIZE]; // 3 or 2 in OpenGL

layout(std140, binding = 10) buffer g_RWBuffArr_Dyn
{
    vec4 data[4];
}g_StorageBuffArr_Dyn[DYNAMIC_BUFF_ARRAY_SIZE]; // 2


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

    // Read from elements 1,2,3
    AllCorrect *= CheckValue(g_StorageBuff_Static.data[1], Buff_Static_Ref);
    AllCorrect *= CheckValue(g_StorageBuff_Mut   .data[2], Buff_Mut_Ref);
    AllCorrect *= CheckValue(g_StorageBuff_Dyn   .data[3], Buff_Dyn_Ref);

    // Write to 0-th element
    vec4 Data = vec4(1.0, 2.0, 3.0, 4.0);
    g_StorageBuff_Static.data[0] = Data;
    g_StorageBuff_Mut   .data[0] = Data;
    g_StorageBuff_Dyn   .data[0] = Data;

    // glslang is not smart enough to unroll the loops even when explicitly told to do so

    AllCorrect *= CheckValue(g_StorageBuffArr_Static[0].data[1], BuffArr_Static_Ref0);

    g_StorageBuffArr_Static[0].data[0] = Data;
#if (STATIC_BUFF_ARRAY_SIZE == 4)
    AllCorrect *= CheckValue(g_StorageBuffArr_Static[1].data[1], BuffArr_Static_Ref1);
    AllCorrect *= CheckValue(g_StorageBuffArr_Static[2].data[2], BuffArr_Static_Ref2);
    AllCorrect *= CheckValue(g_StorageBuffArr_Static[3].data[3], BuffArr_Static_Ref3);

    g_StorageBuffArr_Static[1].data[0] = Data;
    g_StorageBuffArr_Static[2].data[0] = Data;
    g_StorageBuffArr_Static[3].data[0] = Data;
#endif

    AllCorrect *= CheckValue(g_StorageBuffArr_Mut[0].data[1], BuffArr_Mut_Ref0);

    g_StorageBuffArr_Mut[0].data[0] = Data;
#if (MUTABLE_BUFF_ARRAY_SIZE == 3)
    AllCorrect *= CheckValue(g_StorageBuffArr_Mut[1].data[2], BuffArr_Mut_Ref1);
    AllCorrect *= CheckValue(g_StorageBuffArr_Mut[2].data[1], BuffArr_Mut_Ref2);

    g_StorageBuffArr_Mut[1].data[0] = Data;
    g_StorageBuffArr_Mut[2].data[0] = Data;
#endif

    AllCorrect *= CheckValue(g_StorageBuffArr_Dyn[0].data[1], BuffArr_Dyn_Ref0);
    AllCorrect *= CheckValue(g_StorageBuffArr_Dyn[1].data[2], BuffArr_Dyn_Ref1);

    g_StorageBuffArr_Dyn[0].data[0] = Data;
    g_StorageBuffArr_Dyn[1].data[0] = Data;

    return AllCorrect;
}

layout(rgba8, binding = 0) uniform writeonly image2D g_tex2DUAV;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

void main()
{
    ivec2 Dim = imageSize(g_tex2DUAV);
    if (gl_GlobalInvocationID.x >= uint(Dim.x) || gl_GlobalInvocationID.y >= uint(Dim.y))
        return;

    imageStore(g_tex2DUAV, ivec2(gl_GlobalInvocationID.xy), vec4(vec2(gl_GlobalInvocationID.xy % 256u) / 256.0, 0.0, 1.0) * VerifyResources());
}
