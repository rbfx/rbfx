//
// Copyright (c) 2022-2022 the rbfx project.
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

#include "Urho3D/Graphics/Animation.h"
#include "Urho3D/Graphics/Model.h"
#include "Urho3D/Utility/AssetTransformer.h"

namespace Urho3D
{

/// Asset transformer that extracts IK targets from the animation.
class IKTargetExtractor : public AssetTransformer
{
    URHO3D_OBJECT(IKTargetExtractor, AssetTransformer);

public:
    static const ea::string DefaultNewFileName;

    IKTargetExtractor(Context* context);
    ~IKTargetExtractor() override;
    static void RegisterObject(Context* context);

    bool IsApplicable(const AssetTransformerInput& input) override;
    bool Execute(const AssetTransformerInput& input, AssetTransformerOutput& output,
        const AssetTransformerVector& transformers) override;
    bool IsExecutedOnOutput() override { return true; }

private:
    void ExtractAnimation(Animation* sourceAnimation, Animation* destAnimation, Model* model) const;
    ea::string GetNewFileName(const ea::string& fileName) const;
    ea::string GetModelName(Animation* sourceAnimation) const;

    bool extractRotations_{true};
    float sampleRate_{0.0f};
    bool extractToExistingFile_{true};
    bool extractToNewFile_{true};
    ea::string newFileName_{DefaultNewFileName};
    ResourceRef skeletonModel_{Model::GetTypeStatic()};
    StringVariantMap bendTargets_;
};

} // namespace Urho3D
