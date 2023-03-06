#include "Defines.h"

RWTexture2D</*format=rgba8*/ float4> g_tex2DUAV;

#if INTERNAL_MACROS == 1 && EXTERNAL_MACROS == 2
[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 ui2Dim;
	g_tex2DUAV.GetDimensions(ui2Dim.x, ui2Dim.y);
	if (DTid.x >= ui2Dim.x || DTid.y >= ui2Dim.y)
        return;

	g_tex2DUAV[DTid.xy] = float4(float2(DTid.xy % 256u) / 256.0, 0.0, 1.0);
}
#endif
