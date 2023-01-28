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

#include "../../Foundation/ResourceBrowserTab.h"

namespace Urho3D
{

void Foundation_MaterialFactory(Context* context, ResourceBrowserTab* resourceBrowserTab);

/// Camera controller used by Scene View.
class MaterialFactory : public BaseResourceFactory
{
    URHO3D_OBJECT(MaterialFactory, BaseResourceFactory);

public:
    explicit MaterialFactory(Context* context);

    /// Implement BaseResourceFactory.
    /// @{
    ea::string GetDefaultFileName() const override { return "Material.material"; }
    void RenderAuxilary() override;
    void CommitAndClose() override;
    /// @}

private:
    ea::string GetTechniqueName() const;

    enum Type
    {
        Opaque,
        AlphaMask,
        Transparent,
        TransparentFade
    };

    int type_{};
    bool lit_{true};
    bool pbr_{true};
    bool normal_{true};
};

}
