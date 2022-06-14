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

#include <Urho3D/Scene/Serializable.h>
#include <Urho3D/Utility/AssetTransformerHierarchy.h>

namespace Urho3D
{

class ProjectEditor;

/// Manages assets of the project.
class AssetManager : public Object
{
    URHO3D_OBJECT(AssetManager, Object);

public:
    static const ea::string ResourceName;

    explicit AssetManager(Context* context);
    ~AssetManager() override;

    void Update();

private:
    void ReloadPipelines();
    void ProcessAssets();
    StringVector EnumeratePipelineFiles() const;
    StringVector EnumerateAssetFiles() const;

    const WeakPtr<ProjectEditor> projectEditor_;

    ea::string defaultFlavor_{"*"}; // TODO(editor): Make configurable
    bool reloadPipelines_{true};
    bool processAssets_{true};

    SharedPtr<AssetTransformerHierarchy> hierarchy_;
};

}
