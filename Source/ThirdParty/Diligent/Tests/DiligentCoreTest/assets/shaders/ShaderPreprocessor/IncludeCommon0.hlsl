uint PackColor(float4 Color)
{
    return (uint(Color.r * 255) << 24) | (uint(Color.g * 255) << 16) | (uint(Color.b * 255) << 8) | uint(Color.a * 255);
}
