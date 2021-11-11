//
// Copyright (c) 2021 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../CommonUtils.h"
#include "Urho3D/IO/MemoryBuffer.h"
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Resource/Image.h>

namespace Tests
{
namespace
{
const char* DXT1 = "RERTIHwAAAAHEAgAEAAAABAAAACAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAVVZFUgAAAABOVlRUAgE"
                   "CACAAAAAEAAAARFhUMQAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAzCiEKqqqqqh+fES+"
                   "qqqqqO3s4Q6qqqqrfHsoeqqqqqquxoAmqqqqqIPwg3KqqqqogiyADqqqqqoKdgIWqqqqqU1lFEaqqqqpie2Fjqqqqqg7+"
                   "AD6qqqqqbgdgB6qqqqr3388Pqqqqqkb7QCOqqqqqH1weJKqqqqoF/OGbqqqqqg==";
const char* DXT3 =
    "RERTIHwAAAAHEAgAEAAAABAAAAAAAQAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAVVZFUgAAAABOVlRUAgECACAAAAAEAAAARF"
    "hUMwAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAiIiIiIiIiIi0KDQqqqqqqVVVVVVVVVVXbfbp3/////"
    "6qqqqqqqqqqGmy6Yv/////u7u7u7u7u7lgf2B2qqqqqMzMzMzMzMzOoead5/////zMzMzMzMzMzIPQg7Kqqqqru7u7u7u7u7gBjIFv/////"
    "VVVVVVVVVVWClYGVqqqqqkRERERERERETkFPOaqqqqrMzMzMzMzMzAF0AnP/////"
    "7u7u7u7u7u4Jvuq9qqqqqiIiIiIiIiIiaQdJB6qqqqq7u7u7u7u7u/Sf1JeqqqqqREREREREREQitMWy/////"
    "4iIiIiIiIiIH0z+Q6qqqqoREREREREREQPc5Nv/////";
const char* DXT5 =
    "RERTIHwAAAAHEAgAEAAAABAAAAAAAQAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAVVZFUgAAAABOVlRUAgECACAAAAAEAAAARF"
    "hUNQAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAaGgAAAAAAAC0KDQqqqqqqXV0AAAAAAADbfbp3/////"
    "62tAAAAAAAAGmy6Yv/////19QAAAAAAAFgf2B2qqqqqKysAAAAAAACoead5/////zc3AAAAAAAAIPQg7Kqqqqrs7AAAAAAAAABjIFv/////"
    "U1MAAAAAAACClYGVqqqqqkpKAAAAAAAATkFPOaqqqqrPzwAAAAAAAAF0AnP/////"
    "9PQAAAAAAAAJvuq9qqqqqh4eAAAAAAAAaQdJB6qqqqq1tQAAAAAAAPSf1JeqqqqqRkYAAAAAAAAitMWy/////"
    "4aGAAAAAAAAH0z+Q6qqqqoQEAAAAAAAAAPc5Nv/////";
const char* ETC1 = "RERTIHwAAAAHEAgAEAAAABAAAACAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAVVZFUgAAAABOVlRUAgE"
                   "CACAAAAAEAAAARVRDMQAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAIQGgCAAAAAHDg0AL//"
                   "wAAYGDQAgAAAAAY2MAC//8AAHgwOAIAAAAA8IAAAv//AABYYAACAAAAAJCwEAL//wAAQChwAgAAAABwaBAC//8AALjASAL//"
                   "wAAAOhIAgAAAACZ/6oA//8AALBoIAL//wAASID4Av//AADYeCACAAAAAA==";
const char* ETC2 = "RERTIHwAAAAHEAgAEAAAABAAAACAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAVVZFUgAAAABOVlRUAgE"
                   "CACAAAAAEAAAARVRDMgAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAACERPmGRNBImztj8rvjq7x1MmXytm"
                   "ejLLWNWxSO24jbMT406782e+aPe4YEe4YHkIAuZAQvZALsgEkyBcqwHJZDICj6oirqBR06bAW7bCONg19AFd/"
                   "CnfgTAWwVgu6YHZOZ/6oA//8AAFpoDFtqRa0IIwH7o4HyMD9ufgxvgEbvyA==";
const char* PTC2 = "RERTIHwAAAAHEAgAEAAAABAAAABAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAVVZFUgAAAABOVlRUAgE"
                   "CACAAAAAEAAAAUFRDMgAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAADw8PDwKADMKPH08LAFF3Af/////"
                   "4IjmJIPDw8PwDpgofDw8PAINGB1Dw8PD3Ar+1oPDx8P9QCv7w8PDw9yD39C";
const char* PTC4 = "RERTIHwAAAAHEAgAEAAAABAAAACAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAVVZFUgAAAABOVlRUAgE"
                   "CACAAAAAEAAAAUFRDNAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAJwDaNlBQUFAGFnAf/////"
                   "3M03Dj/////oaFwHwAAAAAZIudn/////2UV+ltRm5ZqgGnArAAAAABjLc81/////2F3XWUAAAAAYXagHf////"
                   "+EJLmDAAECAcAq5yAAAAAAj+u8IP////9hC29F/////1Mo8gAAAAAAgw/rdQ==";

// Compare texture area color with expected color
void TestRGBARect(const unsigned char* data, int row, int col, Vector4 expected, bool hasAlpha, float eps, int size = 4)
{
    Vector4 average(0.0f, 0.0f, 0.0f, 0.0f);
    float scale = 0.0f;
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const int offset = ((y + row) * 16 + x + col) * 4;
            average += Vector4(data[offset], data[offset + 1], data[offset + 2], data[offset + 3]);
            scale += 255;
        }
    }
    average /= scale;
    const float diff = hasAlpha ? (average - expected).Length() : Vector3(average - expected).Length();
    REQUIRE(diff <= eps);
};

// Decompress texture and require certain format
SharedPtr<Image> Decompress(SharedPtr<Context> context, const char* data, CompressedFormat format)
{
    auto encodedBytes = DecodeBase64(data);
    auto image = MakeShared<Image>(context);
    MemoryBuffer dds(encodedBytes);
    REQUIRE(image->BeginLoad(dds));
    REQUIRE(image->GetCompressedFormat() == format);
    auto decompressed = image->GetDecompressedImageLevel(0)->ConvertToRGBA();
    REQUIRE(decompressed->GetWidth() == 16);
    REQUIRE(decompressed->GetHeight() == 16);
    return decompressed;
}

// Test default texture areas
void TestTexture(SharedPtr<Context> context, const char* data, CompressedFormat format)
{
    auto decompressed = Decompress(context, data, format);
    const unsigned char* bytes = decompressed->GetData();
    float eps = 3 / 255.0f;
    auto hasA = decompressed->HasAlphaChannel();
    switch (format)
    {
    case CF_DXT3:
    case CF_ETC1:
        eps = 9 / 255.0f;
        break;
    }
    TestRGBARect(bytes, 0, 0, Vector4(0.03529412f, 0.2666667f, 0.4196078f, 0.1019608f), hasA, eps);
    TestRGBARect(bytes, 0, 4, Vector4(0.4588235f, 0.8901961f, 0.8470588f, 0.3647059f), hasA, eps);
    TestRGBARect(bytes, 0, 8, Vector4(0.4039216f, 0.3960784f, 0.8352941f, 0.6784314f), hasA, eps);
    TestRGBARect(bytes, 0, 12, Vector4(0.09411765f, 0.854902f, 0.7803922f, 0.9607843f), hasA, eps);
    TestRGBARect(bytes, 4, 0, Vector4(0.4862745f, 0.2078431f, 0.2313726f, 0.1686275f), hasA, eps);
    TestRGBARect(bytes, 4, 4, Vector4(0.9607843f, 0.5254902f, 0.0f, 0.2156863f), hasA, eps);
    TestRGBARect(bytes, 4, 8, Vector4(0.3686275f, 0.3921569f, 0.0f, 0.9254902f), hasA, eps);
    TestRGBARect(bytes, 4, 12, Vector4(0.5764706f, 0.6980392f, 0.04705882f, 0.3254902f), hasA, eps);
    TestRGBARect(bytes, 8, 0, Vector4(0.254902f, 0.1607843f, 0.4588235f, 0.2901961f), hasA, eps);
    TestRGBARect(bytes, 8, 4, Vector4(0.4509804f, 0.4235294f, 0.05490196f, 0.8117647f), hasA, eps);
    TestRGBARect(bytes, 8, 8, Vector4(0.7450981f, 0.7607843f, 0.3019608f, 0.9568627f), hasA, eps);
    TestRGBARect(bytes, 8, 12, Vector4(0.0f, 0.9333333f, 0.2980392f, 0.1176471f), hasA, eps);
    TestRGBARect(bytes, 12, 0, Vector4(0.5960785f, 0.9921569f, 0.654902f, 0.7098039f), hasA, eps);
    TestRGBARect(bytes, 12, 4, Vector4(0.7137255f, 0.4117647f, 0.1254902f, 0.2745098f), hasA, eps);
    TestRGBARect(bytes, 12, 8, Vector4(0.2745098f, 0.5058824f, 0.9921569f, 0.5254902f), hasA, eps);
    TestRGBARect(bytes, 12, 12, Vector4(0.8745098f, 0.4980392f, 0.1215686f, 0.0627451f), hasA, eps);
}

} // namespace

// Testing compressed images except PVRTC formats due to low image quality at PVRTC samples
TEST_CASE("Image Decompression")
{
    auto context = Tests::CreateCompleteTestContext();
    TestTexture(context, DXT1, CF_DXT1);
    TestTexture(context, DXT3, CF_DXT3);
    TestTexture(context, DXT5, CF_DXT5);
    TestTexture(context, ETC1, CF_ETC1);
    TestTexture(context, ETC2, CF_ETC2_RGB);
}

// Testing few points in the CF_PVRTC_RGBA_2BPP image
TEST_CASE("PVRTC_2BPP Image Decompression")
{
    auto context = Tests::CreateCompleteTestContext();
    auto decompressed = Decompress(context, PTC2, CF_PVRTC_RGBA_2BPP);
    const unsigned char* bytes = decompressed->GetData();
    float eps = 4 / 255.0f;
    auto hasA = decompressed->HasAlphaChannel();
    TestRGBARect(bytes, 0, 0, Vector4(123, 101, 53, 34) / 255.0f, hasA, eps, 1);
    TestRGBARect(bytes, 8, 3, Vector4(89, 28, 102, 65) / 255.0f, hasA, eps, 1);
}

// Testing few points in the CF_PVRTC_RGBA_4BPP image
TEST_CASE("PVRTC_4BPP Image Decompression")
{
    auto context = Tests::CreateCompleteTestContext();
    auto decompressed = Decompress(context, PTC4, CF_PVRTC_RGBA_4BPP);
    const unsigned char* bytes = decompressed->GetData();
    float eps = 5 / 255.0f;
    auto hasA = decompressed->HasAlphaChannel();
    TestRGBARect(bytes, 0, 0, Vector4(101, 103, 72, 25) / 255.0f, hasA, eps, 1);
    TestRGBARect(bytes, 8, 3, Vector4(77, 37, 95, 95) / 255.0f, hasA, eps, 1);
}
} // namespace Tests
