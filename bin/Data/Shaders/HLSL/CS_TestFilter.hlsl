/*
    Trivial filtering example that just dims the input by 50%
*/

#define DIM 64.0

// computeDevice->SetReadTexture(myTex, 0)
Texture2D srcTex : register(t0);

// computeDevice->SetWriteTexture(myTex, 1)
RWTexture2D<float4> outputTexture : register(u1);

[numthreads(8,8,1)]
void CS(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    float2 uvCoords = float2(dispatchThreadId.x, dispatchThreadId.y) / DIM;
    float4 pixel = srcTex.Load(int3(dispatchThreadId.xy, 0));
    
    pixel.rgb *= 0.5;
    
    outputTexture[dispatchThreadId.xy] = pixel;
}