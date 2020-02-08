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
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/SystemUI/SystemUI.h>

#include "ModelInspector.h"
#include "Editor.h"


namespace Urho3D
{

ModelInspector::ModelInspector(Context* context)
    : PreviewInspector(context)
{
}

void ModelInspector::SetInspected(Object* inspected)
{
    assert(inspected->IsInstanceOf<Model>());
    InspectorProvider::SetInspected(inspected);
    SetModel(static_cast<Model*>(inspected));
}

void ModelInspector::RenderInspector(const char* filter)
{
    if (inspected_.Expired())
        return;

    if (ui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen))
    {
        RenderPreview();
        if (auto* staticModel = node_->GetComponent<StaticModel>())
        {
            if (Model* modelResource = staticModel->GetModel())
            {
                const char* resourceName = modelResource->GetName().c_str();
                ui::SetCursorPosX((ui::GetContentRegionMax().x - ui::CalcTextSize(resourceName).x) / 2);
                ui::TextUnformatted(resourceName);
                ui::Separator();
            }
        }
    }
}

}
