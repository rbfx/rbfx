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
#include <Urho3D/Container/Str.h>
#include <Toolbox/SystemUI/Widgets.h>

#include "Editor.h"
#include "Pipeline/Pipeline.h"
#include "Pipeline/Asset.h"
#include "Inspector/AssetInspector.h"
#include "Tabs/InspectorTab.h"

namespace Urho3D
{

AssetInspector::AssetInspector(Context* context)
    : Object(context)
{
    auto* editor = GetSubsystem<Editor>();
    editor->onInspect_.Subscribe(this, &AssetInspector::RenderInspector);
}

void AssetInspector::RenderInspector(InspectArgs& args)
{
    auto* asset = args.object_.Expired() ? nullptr : args.object_->Cast<Asset>();
    if (asset == nullptr)
        return;

    args.handledTimes_++;
    ui::IdScope idScope(asset);
    auto* pipeline = GetSubsystem<Pipeline>();
    bool tabBarStarted = false;
    bool save = false;
    bool headerRendered = false;

    // Use flavors list from the pipeline because it is sorted. Asset::importers_ is unordered.
    for (const SharedPtr<Flavor>& flavor : pipeline->GetFlavors())
    {
        bool tabStarted = false;
        bool tabVisible = false;

        for (AssetImporter* importer : asset->GetImporters(flavor))
        {
            bool importerSupportsFiles = false;
            if (asset->IsMetaAsset())
            {
                // This is a meta-asset pointing to a directory. Such assets are used to hold importer settings
                // for downstream importers to inherit from. Show all importers for directories.
                // TODO: Look into subdirectories and show only importers valid for contents of the folder.
                importerSupportsFiles = true;
            }
            else
            {
                // This is a real asset. Show only those importers that can import asset itself or any of it's
                // byproducts.
                importerSupportsFiles = importer->Accepts(asset->GetResourcePath());
                for (const auto& siblingImporter : asset->GetImporters(flavor))
                {
                    if (importer == siblingImporter)
                        continue;

                    for (const auto& byproduct : siblingImporter->GetByproducts())
                    {
                        importerSupportsFiles |= importer->Accepts(byproduct);
                        if (importerSupportsFiles)
                            break;
                    }
                    if (importerSupportsFiles)
                        break;
                }
            }

            if (importerSupportsFiles)
            {
                // Defer rendering of tab bar and tabs until we know that we have compatible importers. As a result if
                // file is not supported by any importer - tab bar with flavors and no content will not be shown.
                if (!headerRendered)
                {
                    ui::TextCentered(Format("Asset: {}", asset->GetName()).c_str());
                    ui::Separator();
                    headerRendered = true;
                }

                if (!tabBarStarted)
                {
                    ui::BeginTabBar(Format("###{}", (void*)this).c_str(), ImGuiTabBarFlags_None);
                    tabBarStarted = true;
                }

                if (!tabStarted)
                {
                    tabStarted = true;
                    tabVisible = ui::BeginTabItem(flavor->GetName().c_str());
                    if (tabVisible)
                        ui::SetHelpTooltip("Pipeline flavor");
                }

                if (tabVisible)
                {
                    if (importer->GetNumAttributes() > 0 &&
                        ui::CollapsingHeader(importer->GetTypeName().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        RenderAttributes(importer, args.filter_, importer);
                        save |= importer->IsModified();
                    }
                }
            }
        }

        if (tabVisible)
            ui::EndTabItem();
    }

    if (tabBarStarted)
        ui::EndTabBar();

    if (save)
        asset->Save();
}

}
