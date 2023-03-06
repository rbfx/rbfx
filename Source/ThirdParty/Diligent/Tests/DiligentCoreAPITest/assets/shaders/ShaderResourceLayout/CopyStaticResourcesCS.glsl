layout(std140, binding = 0) buffer g_RWBuff_0
{
    vec4 data[4];
}g_StorageBuff0;

layout(std140, binding = 1) buffer g_RWBuff_1
{
    vec4 data[4];
}g_StorageBuff1;


layout(std140, binding = 2) buffer g_RWBuff_2
{
    vec4 data[4];
}g_StorageBuff2;

layout(rgba8, binding = 3) uniform image2D g_RWTex2D_0;
layout(rgba8, binding = 4) uniform image2D g_RWTex2D_1;
layout(rgba8, binding = 5) uniform image2D g_RWTex2D_2;

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
    AllCorrect *= CheckValue(g_StorageBuff0.data[1], Buff_0_Ref);
    AllCorrect *= CheckValue(g_StorageBuff1.data[2], Buff_1_Ref);
    AllCorrect *= CheckValue(g_StorageBuff2.data[3], Buff_2_Ref);

    // Write to 0-th element
    vec4 Data = vec4(1.0, 2.0, 3.0, 4.0);
    g_StorageBuff0.data[0] = Data;
    g_StorageBuff1.data[0] = Data;
    g_StorageBuff2.data[0] = Data;

    AllCorrect *= CheckValue(imageLoad(g_RWTex2D_0, ivec2(10, 12)), RWTex2D_0_Ref);
    AllCorrect *= CheckValue(imageLoad(g_RWTex2D_1, ivec2(14, 17)), RWTex2D_1_Ref);
    AllCorrect *= CheckValue(imageLoad(g_RWTex2D_2, ivec2(31, 24)), RWTex2D_2_Ref);

    imageStore(g_RWTex2D_0, ivec2(0, 0), vec4(1.0, 2.0, 3.0, 4.0));
    imageStore(g_RWTex2D_1, ivec2(0, 0), vec4(1.0, 2.0, 3.0, 4.0));
    imageStore(g_RWTex2D_2, ivec2(0, 0), vec4(1.0, 2.0, 3.0, 4.0));

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
