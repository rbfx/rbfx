// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/ResourceBrowserTab/AssetPipelineFactory.h"

#include "../../Assets/ModelImporter.h"

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Utility/AssetPipeline.h>

namespace Urho3D
{

void Foundation_AssetPipelineFactory(Context* context, ResourceBrowserTab* resourceBrowserTab)
{
    resourceBrowserTab->AddFactory(MakeShared<AssetPipelineFactory>(context));
}

AssetPipelineFactory::AssetPipelineFactory(Context* context)
    : BaseResourceFactory(context, 0, "Asset Pipeline")
{
}

void AssetPipelineFactory::RenderAuxilary()
{
    ui::Separator();

    ui::Checkbox("Model Importer", &modelImporter_);
    if (ui::IsItemHovered())
        ui::SetTooltip("Add default ModelImporter to the pipeline.");

    ui::Separator();
}

void AssetPipelineFactory::CommitAndClose()
{
    auto cache = GetSubsystem<ResourceCache>();

    auto pipeline = MakeShared<AssetPipeline>(context_);

    if (modelImporter_)
    {
        auto modelImporter = MakeShared<ModelImporter>(context_);
        pipeline->AddTransformer(modelImporter);
    }

    pipeline->SaveFile(GetFinalFileName());
}

}
