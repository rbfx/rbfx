float4 UnpackColor(uint Color) {
    float4 Result;
    Result.r = float((Color >> 24) & 0x000000FF) / 255.0f;
    Result.g = float((Color >> 16) & 0x000000FF) / 255.0f;
    Result.b = float((Color >> 8) & 0x000000FF) / 255.0f;
    Result.a = float((Color >> 0) & 0x000000FF) / 255.0f;
    return saturate(Result);
}
