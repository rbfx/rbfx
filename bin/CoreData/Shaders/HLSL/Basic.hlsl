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

#ifdef COMPILEVS
struct VertexInput {
    float4 iPos : POSITION;
    #ifdef DIFFMAP
    float2 iTexCoord: TEXCOORD0;
    #endif
    #ifdef VERTEXCOLOR
    float4 iColor: COLOR0;
    #endif
};
struct VertexOutput {
    #ifdef DIFFMAP
    float2 oTexCoord : TEXCOORD0;
    #endif
    #ifdef VERTEXCOLOR
    float4 oColor: COLOR0;
    #endif
    float4 oPos : SV_POSITION;
};

VertexOutput VS(VertexInput input)
{
    VertexOutput output;
    float3 worldPos = mul(input.iPos, cModel);
    output.oPos = mul(float4(worldPos, 1.0), cViewProj);

    #ifdef VERTEXCOLOR
        output.oColor = input.iColor;
    #endif
    #ifdef DIFFMAP
        output.oTexCoord = input.iTexCoord;
    #endif
    return output;
}
#endif

#ifdef COMPILEPS
struct VertexInput {
    #if defined(DIFFMAP) || defined(ALPHAMAP)
    float2 iTexCoord: TEXCOORD0;
    #endif
    #ifdef VERTEXCOLOR
    float4 iColor: COLOR0;
    #endif
    float4 iPos : SV_POSITION;
};
float4 PS(VertexInput input) : SV_TARGET
{
    float4 diffColor = 1.0;

    #ifdef VERTEXCOLOR
        diffColor *= input.iColor;
    #endif

    #ifdef DIFFMAP
        float4 diffInput = tDiffMap.Sample(sDiffMap, input.iTexCoord);
        #ifdef ALPHAMASK
            if (diffInput.a < 0.5)
                discard;
        #endif
        diffColor *= diffInput;
    #endif
    #ifdef ALPHAMAP
        float alphaInput = tDiffMap.Sample(sDiffMap, input.iTexCoord).a;
        diffColor.a *= alphaInput;
    #endif

    #ifdef URHO3D_LINEAR_OUTPUT
        diffColor.rgb *= diffColor.rgb * (diffColor.rgb * 0.305306011 + 0.682171111) + 0.012522878;
    #endif
    return diffColor;
}
#endif