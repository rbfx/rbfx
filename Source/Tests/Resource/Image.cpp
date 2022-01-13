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

// These blobs contain texture "SourceAssets/UT_Image.png"
const char* PNG = "iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsMAAA7DAcdvqGQAAABzSURBVDhPY+B0yZZCxqWPb8Qi4/TUq2uRscSt41+R8XAwoMbUWhsZf21jMEfGcSkMb5Dx5E08wch4OBjgqFnqhYyLc/jOI+N9h3y/IGOGdz5yKHgYGDDj7/KtyHhbpoIbMnZr/NuGjO/Xywsg4yFvgLwAAN1K3CAi1LVPAAAAAElFTkSuQmCC";
const char* DXT1 = "RERTIHwAAAAHEAgAEAAAABAAAACAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAVVZFUgAAAABOVlRUAgECACAAAAAEAAAARFhUMQAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAzCiEKqqqqqh+fES+qqqqqO3s4Q6qqqqrfHsoeqqqqqquxoAmqqqqqIPwg3KqqqqogiyADqqqqqoKdgIWqqqqqU1lFEaqqqqpie2Fjqqqqqg7+AD6qqqqqbgdgB6qqqqr3388Pqqqqqkb7QCOqqqqqH1weJKqqqqoF/OGbqqqqqg==";
const char* DXT3 = "RERTIHwAAAAHEAgAEAAAABAAAAAAAQAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAVVZFUgAAAABOVlRUAgECACAAAAAEAAAARFhUMwAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAiIiIiIiIiIi0KDQqqqqqqVVVVVVVVVVXbfbp3/////6qqqqqqqqqqGmy6Yv/////u7u7u7u7u7lgf2B2qqqqqMzMzMzMzMzOoead5/////zMzMzMzMzMzIPQg7Kqqqqru7u7u7u7u7gBjIFv/////VVVVVVVVVVWClYGVqqqqqkRERERERERETkFPOaqqqqrMzMzMzMzMzAF0AnP/////7u7u7u7u7u4Jvuq9qqqqqiIiIiIiIiIiaQdJB6qqqqq7u7u7u7u7u/Sf1JeqqqqqREREREREREQitMWy/////4iIiIiIiIiIH0z+Q6qqqqoREREREREREQPc5Nv/////";
const char* DXT5 = "RERTIHwAAAAHEAgAEAAAABAAAAAAAQAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAVVZFUgAAAABOVlRUAgECACAAAAAEAAAARFhUNQAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAaGgAAAAAAAC0KDQqqqqqqXV0AAAAAAADbfbp3/////62tAAAAAAAAGmy6Yv/////19QAAAAAAAFgf2B2qqqqqKysAAAAAAACoead5/////zc3AAAAAAAAIPQg7Kqqqqrs7AAAAAAAAABjIFv/////U1MAAAAAAACClYGVqqqqqkpKAAAAAAAATkFPOaqqqqrPzwAAAAAAAAF0AnP/////9PQAAAAAAAAJvuq9qqqqqh4eAAAAAAAAaQdJB6qqqqq1tQAAAAAAAPSf1JeqqqqqRkYAAAAAAAAitMWy/////4aGAAAAAAAAH0z+Q6qqqqoQEAAAAAAAAAPc5Nv/////";
const char* ETC1 = "RERTIHwAAAAHEAgAEAAAABAAAACAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAVVZFUgAAAABOVlRUAgECACAAAAAEAAAARVRDMQAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAIQGgCAAAAAHDg0AL//wAAYGDQAgAAAAAY2MAC//8AAHgwOAIAAAAA8IAAAv//AABYYAACAAAAAJCwEAL//wAAQChwAgAAAABwaBAC//8AALjASAL//wAAAOhIAgAAAACZ/6oA//8AALBoIAL//wAASID4Av//AADYeCACAAAAAA==";
const char* ETC2 = "RERTIHwAAAAHEAgAEAAAABAAAACAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAVVZFUgAAAABOVlRUAgECACAAAAAEAAAARVRDMgAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAACERPmGRNBImztj8rvjq7x1MmXytmejLLWNWxSO24jbMT406782e+aPe4YEe4YHkIAuZAQvZALsgEkyBcqwHJZDICj6oirqBR06bAW7bCONg19AFd/CnfgTAWwVgu6YHZOZ/6oA//8AAFpoDFtqRa0IIwH7o4HyMD9ufgxvgEbvyA==";
const char* PVRTC2 = "RERTIHwAAAAHEAgAEAAAABAAAABAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAVVZFUgAAAABOVlRUAgECACAAAAAEAAAAUFRDMgAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAADw8PDwKADMKPH08LAFF3Af/////4IjmJIPDw8PwDpgofDw8PAINGB1Dw8PD3Ar+1oPDx8P9QCv7w8PDw9yD39C";
const char* PVRTC4 = "RERTIHwAAAAHEAgAEAAAABAAAACAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAVVZFUgAAAABOVlRUAgECACAAAAAEAAAAUFRDNAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAJwDaNlBQUFAGFnAf/////3M03Dj/////oaFwHwAAAAAZIudn/////2UV+ltRm5ZqgGnArAAAAABjLc81/////2F3XWUAAAAAYXagHf////+EJLmDAAECAcAq5yAAAAAAj+u8IP////9hC29F/////1Mo8gAAAAAAgw/rdQ==";

SharedPtr<Image> ReadImage(Context* context, const char* data, CompressedFormat format)
{
    auto encodedBytes = DecodeBase64(data);
    auto image = MakeShared<Image>(context);
    MemoryBuffer buffer(encodedBytes);
    REQUIRE(image->BeginLoad(buffer));
    REQUIRE(image->GetCompressedFormat() == format);
    auto decompressed = image->GetDecompressedImageLevel(0)->ConvertToRGBA();
    return decompressed;
}

float CompareImages(const Image& lhs, const Image& rhs, bool compareAlpha, float threshold = 0.0f)
{
    REQUIRE(lhs.GetSize() == rhs.GetSize());
    double errorSum = 0.0;
    const IntVector2 size = lhs.GetSize().ToVector2();
    for (const IntVector2 index : IntRect(IntVector2::ZERO, size))
    {
        const Color& lhsColor = lhs.GetPixel(index.x_, index.y_);
        const Color& rhsColor = rhs.GetPixel(index.x_, index.y_);
        const Vector4 delta = lhsColor.ToVector4() - rhsColor.ToVector4();
        const float maxDelta = ea::max({ Abs(delta.x_), Abs(delta.y_), Abs(delta.z_), compareAlpha ? Abs(delta.w_) : 0.0f });
        if (maxDelta >= threshold)
            errorSum += ea::min(1.0f, maxDelta * maxDelta);
    }
    return static_cast<float>(Sqrt(errorSum / (size.x_ * size.y_)));
}

} // namespace

TEST_CASE("DXT, ETC and PVRTC images are decompressed")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    const auto imageReference = ReadImage(context, PNG, CF_NONE);
    const auto imageDXT1 = ReadImage(context, DXT1, CF_DXT1);
    const auto imageDXT3 = ReadImage(context, DXT3, CF_DXT3);
    const auto imageDXT5 = ReadImage(context, DXT5, CF_DXT5);
    const auto imageETC1 = ReadImage(context, ETC1, CF_ETC1);
    const auto imageETC2 = ReadImage(context, ETC2, CF_ETC2_RGB);
    const auto imagePVRTC2 = ReadImage(context, PVRTC2, CF_PVRTC_RGBA_2BPP);
    const auto imagePVRTC4 = ReadImage(context, PVRTC4, CF_PVRTC_RGBA_4BPP);

    // Test that reference image is expected
    REQUIRE(imageReference->GetSize() == IntVector3{ 16, 16, 1 });
    REQUIRE(imageReference->GetPixel(4, 12) == 0x46B66920_argb);

    // Test that other images match reference image
    REQUIRE(CompareImages(*imageReference, *imageDXT1, false) < 0.02f);
    REQUIRE(CompareImages(*imageReference, *imageDXT3, true)  < 0.02f);
    REQUIRE(CompareImages(*imageReference, *imageDXT5, true)  < 0.02f);
    REQUIRE(CompareImages(*imageReference, *imageETC1, false) < 0.02f);
    REQUIRE(CompareImages(*imageReference, *imageETC2, false) < 0.02f);

    REQUIRE(CompareImages(*imageReference, *imagePVRTC2, false) < 0.25f);
    REQUIRE(CompareImages(*imageReference, *imagePVRTC4, false) < 0.15f);
}

} // namespace Tests
