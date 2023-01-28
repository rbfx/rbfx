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

#include "../../Foundation/ResourceBrowserTab/SceneFactory.h"

#include "../../Project/CreateDefaultScene.h"

namespace Urho3D
{

void Foundation_SceneFactory(Context* context, ResourceBrowserTab* resourceBrowserTab)
{
    resourceBrowserTab->AddFactory(MakeShared<SceneFactory>(context, true));
    resourceBrowserTab->AddFactory(MakeShared<SceneFactory>(context, false));
}

SceneFactory::SceneFactory(Context* context, bool isPrefab)
    : BaseResourceFactory(context, 0, isPrefab ? "Prefab" : "Scene")
    , isPrefab_(isPrefab)
{
}

void SceneFactory::RenderAuxilary()
{
    if (!isPrefab_)
    {
        ui::Separator();

        ui::Checkbox("High Quality", &highQuality_);
        if (ui::IsItemHovered())
            ui::SetTooltip("Use renderer settings for high picture quality");

        ui::Checkbox("Default Objects", &defaultObjects_);
        if (ui::IsItemHovered())
            ui::SetTooltip("Add default light, environment and teapot to the scene.");
    }

    ui::Separator();
}

void SceneFactory::CommitAndClose()
{
    DefaultSceneParameters params;
    params.highQuality_ = highQuality_;
    params.createObjects_ = defaultObjects_;
    params.isPrefab_ = isPrefab_;

    CreateDefaultScene(context_, GetFinalFileName(), params);
}

}
