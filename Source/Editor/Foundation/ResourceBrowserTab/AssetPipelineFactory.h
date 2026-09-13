// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Foundation/ResourceBrowserTab.h"

namespace Urho3D
{

void Foundation_AssetPipelineFactory(Context* context, ResourceBrowserTab* resourceBrowserTab);

/// Camera controller used by Scene View.
class AssetPipelineFactory : public BaseResourceFactory
{
    URHO3D_OBJECT(AssetPipelineFactory, BaseResourceFactory);

public:
    explicit AssetPipelineFactory(Context* context);

    /// Implement BaseResourceFactory.
    /// @{
    ea::string GetDefaultFileName() const override { return "Default.assetpipeline"; }
    void RenderAuxilary() override;
    void CommitAndClose() override;
    /// @}

private:
    bool modelImporter_{false};
};

}
