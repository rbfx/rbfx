// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Assets/DilateTextureTransformer.h"

#include <Urho3D/Resource/Image.h>

namespace Urho3D
{

namespace
{

const StringVector defaultPatterns{R"(.+\.png)"};

void DilateImage(Image* image, float alphaThreshold)
{
    const int width = image->GetWidth();
    const int height = image->GetHeight();
    const int numPixels = width * height;
    ea::vector<Color> pixels;
    pixels.resize(numPixels);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
            pixels[y * width + x] = image->GetPixel(x, y);
    }

    // Queue all opaque pixels.
    ea::vector<int> opaqueIndex(numPixels, -1);
    ea::vector<int> queue;
    for (int i = 0; i < numPixels; ++i)
    {
        if (pixels[i].a_ > alphaThreshold)
        {
            opaqueIndex[i] = i;
            queue.push_back(i);
        }
    }

    // Multi-source BFS.
    for (unsigned i = 0; i < queue.size(); ++i)
    {
        const int index = queue[i];
        const int x = index % width;
        const int y = index / width;
        const Color& color = pixels[opaqueIndex[index]];

        static constexpr ea::array<IntVector2, 4> offsets{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};
        for (const IntVector2& offset : offsets)
        {
            const int neighborX = x + offset.x_;
            const int neighborY = y + offset.y_;
            if (neighborX < 0 || neighborX >= width || neighborY < 0 || neighborY >= height)
                continue;

            const int neighbor = neighborY * width + neighborX;
            if (opaqueIndex[neighbor] == -1)
            {
                opaqueIndex[neighbor] = opaqueIndex[index];
                queue.push_back(neighbor);

                Color neighborColor = color;
                neighborColor.a_ = pixels[neighbor].a_;
                image->SetPixel(neighborX, neighborY, neighborColor);
            }
        }
    }
}

} // namespace

void Assets_DilateTextureTransformer(Context* context, Project* project)
{
    if (!context->IsReflected<DilateTextureTransformer>())
        DilateTextureTransformer::RegisterObject(context);
}

DilateTextureTransformer::DilateTextureTransformer(Context* context)
    : AssetTransformer(context)
    , fileNamePatterns_(defaultPatterns)
{
}

void DilateTextureTransformer::RegisterObject(Context* context)
{
    context->AddFactoryReflection<DilateTextureTransformer>(Category_Transformer);

    // clang-format off
    URHO3D_ATTRIBUTE_EX("File Name Regex", StringVector, fileNamePatterns_, MarkRegexesDirty, defaultPatterns, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Alpha Threshold", float, alphaThreshold_, 0.0f, AM_DEFAULT);
    // clang-format on
}

bool DilateTextureTransformer::IsApplicable(const AssetTransformerInput& input)
{
    EnsureInitialized();

    const ea::string& resourceName = input.resourceName_;
    for (const std::regex& r : fileNamePatternsRegex_)
    {
        if (std::regex_match(resourceName.begin(), resourceName.end(), r))
            return true;
    }

    return false;
}

bool DilateTextureTransformer::Execute(
    const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers)
{
    const auto image = MakeShared<Image>(context_);
    if (!image->LoadFile(input.inputFileName_))
        return false;

    if (image->IsCompressed() || image->GetDepth() > 1)
    {
        URHO3D_LOGERROR("Dilation is not supported for image of this type");
        return false;
    }

    DilateImage(image, alphaThreshold_);

    if (!image->SavePNG(input.inputFileName_))
        return false;

    output.sourceModified_ = true;
    return true;
}

void DilateTextureTransformer::MarkRegexesDirty()
{
    fileNamePatternsRegex_.clear();
}

void DilateTextureTransformer::EnsureInitialized()
{
    if (!fileNamePatternsRegex_.empty())
        return;

    for (const ea::string& pattern : fileNamePatterns_)
    {
        try
        {
            const std::regex r{pattern.c_str(), std::regex_constants::icase};
            fileNamePatternsRegex_.push_back(r);
        }
        catch (const std::regex_error& e)
        {
            URHO3D_LOGERROR("Invalid file name regex: {}", e.what());
        }
    }
}

} // namespace Urho3D
