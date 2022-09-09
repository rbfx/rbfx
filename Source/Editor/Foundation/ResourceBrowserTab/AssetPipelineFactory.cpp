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
