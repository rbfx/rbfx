#version 430

layout(binding = 0)
uniform DispatchData
{
    uint dispatchWidth;
};

layout(std430, binding = 0)
buffer destBuffer 
{
    vec4 destData[];
};

layout(std430, binding = 1)
buffer srcBuffer
{
    vec4 srcData[];
};

layout(local_size_x = 16, local_size_y = 1, local_size_z = 1) in;
void CS()
{
    uvec3 dtid = gl_GlobalInvocationID.xyz;
    
    if (dtid.x > dispatchWidth)
        return;
        
    destData[dtid.x] = srcData[dtid.x];
}