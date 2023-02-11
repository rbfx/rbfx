cbuffer CameraVS : register(b1)
{
    float4x4 cViewProj;
}

cbuffer ObjectVS : register(b5)
{
    float4x3 cModel;
}

Texture2D tDiffMap : register(t0);
SamplerState sDiffMap : register(s0);

#define Sample2D(tex, uv) t##tex.Sample(s##tex, uv)

void VS(float4 iPos : POSITION,
    #ifdef DIFFMAP
        float2 iTexCoord : TEXCOORD0,
    #endif
    #ifdef VERTEXCOLOR
        float4 iColor : COLOR0,
    #endif
    #ifdef DIFFMAP
        out float2 oTexCoord : TEXCOORD0,
    #endif
    #ifdef VERTEXCOLOR
        out float4 oColor : COLOR0,
    #endif
    out float4 oPos : SV_POSITION)
{
    float3 worldPos = mul(iPos, cModel);
    oPos = mul(float4(worldPos, 1.0), cViewProj);

    #ifdef VERTEXCOLOR
        oColor = iColor;
    #endif
    #ifdef DIFFMAP
        oTexCoord = iTexCoord;
    #endif
}

void PS(
    #if defined(DIFFMAP) || defined(ALPHAMAP)
        float2 iTexCoord : TEXCOORD0,
    #endif
    #ifdef VERTEXCOLOR
        float4 iColor : COLOR0,
    #endif
    out float4 oColor : SV_TARGET)
{
    float4 diffColor = 1.0;

    #ifdef VERTEXCOLOR
        diffColor *= iColor;
    #endif

    #ifdef DIFFMAP
        float4 diffInput = Sample2D(DiffMap, iTexCoord);
        #ifdef ALPHAMASK
            if (diffInput.a < 0.5)
                discard;
        #endif
        diffColor *= diffInput;
    #endif
    #ifdef ALPHAMAP
        float alphaInput = Sample2D(DiffMap, iTexCoord).a;
        diffColor.a *= alphaInput;
    #endif

    #ifdef URHO3D_LINEAR_OUTPUT
        diffColor.rgb *= diffColor.rgb * (diffColor.rgb * 0.305306011 + 0.682171111) + 0.012522878;
    #endif
    oColor = diffColor;
}
