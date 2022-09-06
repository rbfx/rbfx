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

#pragma once

#include "../Resource/ResourceCache.h"
#include "../Utility/AssetTransformer.h"

namespace Urho3D
{

/// Resource containing an array of AssetTransformer-s.
class URHO3D_API AssetPipeline : public Resource
{
    URHO3D_OBJECT(AssetPipeline, Resource);

public:
    explicit AssetPipeline(Context* context);
    static bool CheckExtension(const ea::string& fileName);

    void AddTransformer(AssetTransformer* transformer);
    void RemoveTransformer(AssetTransformer* transformer);
    void ReorderTransformer(AssetTransformer* transformer, unsigned index);

    /// Implement Resource.
    /// @{
    void SerializeInBlock(Archive& archive) override;
    bool BeginLoad(Deserializer& source) override;
    bool Save(Serializer& dest) const override;
    /// @}

    const ea::vector<SharedPtr<AssetTransformer>>& GetTransformers() const { return transformers_; }
    const ea::vector<AssetTransformerDependency>& GetDependencies() const { return dependencies_; }

private:
    ea::vector<SharedPtr<AssetTransformer>> transformers_;
    ea::vector<AssetTransformerDependency> dependencies_;
};

}
