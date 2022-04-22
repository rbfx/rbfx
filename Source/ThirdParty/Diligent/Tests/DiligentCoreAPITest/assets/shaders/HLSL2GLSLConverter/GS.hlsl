
struct VSOutput
{
    float4 Pos : SV_Position;
};

struct GSOutput
{
    float4 Pos    : SV_Position;
    uint   PrimId : PrimId;
};

[maxvertexcount(3)]
void main(triangle VSOutput In[3],
          inout TriangleStream<GSOutput> triStream,
          uint PrimID : SV_PrimitiveID)
{
    GSOutput Out;
    Out.PrimId = PrimID;

    Out.Pos = In[0].Pos;
    triStream.Append(Out);

    Out.Pos = In[1].Pos;
    triStream.Append(Out);

    Out.Pos = In[2].Pos;
    triStream.Append(Out);
}
