cbuffer FrameVS : register(b0)
{
    float4x3 cMatrix;
    float4 cColor;
}

void VS(float4 iPos : POSITION,
    out float4 oPos : SV_POSITION)
{
    oPos = float4(mul(iPos, cMatrix), 1.0);
}

void PS(out float4 oColor : SV_TARGET)
{
    oColor = cColor;
}
