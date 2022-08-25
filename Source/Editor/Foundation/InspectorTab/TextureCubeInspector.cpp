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

#include "../../Foundation/InspectorTab/TextureCubeInspector.h"

#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/TextureCube.h>
#include <Urho3D/SystemUI/SceneWidget.h>
#include <Urho3D/SystemUI/TextureCubeInspectorWidget.h>

namespace Urho3D
{

void Foundation_TextureCubeInspector(Context* context, InspectorTab* inspectorTab)
{
    inspectorTab->RegisterAddon<TextureCubeInspector>(inspectorTab->GetProject());
}

TextureCubeInspector::TextureCubeInspector(Project* project)
    : BaseClassName(project)
{
}

StringHash TextureCubeInspector::GetResourceType() const
{
    return TextureCube::GetTypeStatic();
}

SharedPtr<ResourceInspectorWidget> TextureCubeInspector::MakeInspectorWidget(const ResourceVector& resources)
{
    return MakeShared<TextureCubeInspectorWidget>(context_, resources);
}

SharedPtr<BaseWidget> TextureCubeInspector::MakePreviewWidget(Resource* resource)
{
    auto sceneWidget = MakeShared<SceneWidget>(context_);
    sceneWidget->CreateDefaultScene();
    sceneWidget->SetSkyboxTexture(static_cast<Texture*>(resource));
    return sceneWidget;
}
} // namespace Urho3D
