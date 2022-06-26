//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../IO/ArchiveSerialization.h"
#include "../IO/Base64Archive.h"
#include "../IO/Log.h"
#include "../Utility/AssetTransformer.h"

#include <EASTL/sort.h>
#include <EASTL/unordered_set.h>

namespace Urho3D
{

AssetTransformerInput::AssetTransformerInput(const ea::string& flavor, const ea::string& resourceName, const ea::string& inputFileName)
    : flavor_(flavor)
    , resourceName_(resourceName)
    , fileName_(inputFileName)
{
}

AssetTransformerInput::AssetTransformerInput(const AssetTransformerInput& other, const ea::string& outputFileName)
    : flavor_(other.flavor_)
    , resourceName_(other.resourceName_)
    , fileName_(other.fileName_)
    , outputFileName_(outputFileName)
{
}

void AssetTransformerInput::SerializeInBlock(Archive& archive)
{
    SerializeValue(archive, "Flavor", flavor_);
    SerializeValue(archive, "ResourceName", resourceName_);
    SerializeValue(archive, "FileName", fileName_);
    SerializeValue(archive, "OutputFileName", outputFileName_);
}

AssetTransformerInput AssetTransformerInput::FromBase64(const ea::string& base64)
{
    Base64InputArchive archive(nullptr, base64);
    AssetTransformerInput result;
    SerializeValue(archive, "Input", result);
    return result;
}

ea::string AssetTransformerInput::ToBase64() const
{
    Base64OutputArchive archive(nullptr);
    SerializeValue(archive, "Input", const_cast<AssetTransformerInput&>(*this));
    return archive.GetBase64();
}

void AssetTransformerOutput::SerializeInBlock(Archive& archive)
{
    SerializeValue(archive, "AppliedTransformers", appliedTransformers_);
}

AssetTransformerOutput AssetTransformerOutput::FromBase64(const ea::string& base64)
{
    Base64InputArchive archive(nullptr, base64);
    AssetTransformerOutput result;
    SerializeValue(archive, "Output", result);
    return result;
}

ea::string AssetTransformerOutput::ToBase64() const
{
    Base64OutputArchive archive(nullptr);
    SerializeValue(archive, "Output", const_cast<AssetTransformerOutput&>(*this));
    return archive.GetBase64();
}

AssetTransformer::AssetTransformer(Context* context)
    : Serializable(context)
{
}

bool AssetTransformer::IsApplicable(const AssetTransformerInput& input, const AssetTransformerVector& transformers)
{
    for (AssetTransformer* transformer : transformers)
    {
        if (transformer->IsApplicable(input))
            return true;
    }
    return false;
}

bool AssetTransformer::Execute(const AssetTransformerInput& input, const AssetTransformerVector& transformers, AssetTransformerOutput& output)
{
    unsigned index = 0;
    for (AssetTransformer* transformer : transformers)
    {
        if (transformer->IsApplicable(input))
        {
            if (!transformer->Execute(input, output))
                return false;
            output.appliedTransformers_.push_back(index);
        }
        ++index;
    }
    return true;
}

void AssetTransformer::SetFlavor(const ea::string& value)
{
    if (value.empty())
        flavor_ = "*";
    else if (!value.starts_with("*"))
        flavor_ = Format("*.{}", value);
    else
        flavor_ = value;
}

}
