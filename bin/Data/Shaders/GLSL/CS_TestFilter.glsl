#version 430

// computeDevice->SetReadTexture(myTex, 0)
layout(binding = 0)
uniform sampler2D readTexture;

// computeDevice->SetWriteTexture(myTex, 1)
layout(binding = 1, rgba8)
uniform image2D writeTexture;

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void CS()
{
    ivec2 writeCoord = ivec2(gl_GlobalInvocationID.xy);
    
    vec4 pixel = texelFetch(readTexture, writeCoord, 0);
    pixel.rgb *= 0.5;
    
    imageStore(writeTexture, writeCoord, pixel);
}