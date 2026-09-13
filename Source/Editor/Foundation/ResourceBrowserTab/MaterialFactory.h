// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
