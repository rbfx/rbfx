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

#include "../../Foundation/InspectorTab/Texture2DInspector.h"

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/SystemUI/Texture2DWidget.h>
#include <Urho3D/SystemUI/Texture2DInspectorWidget.h>

namespace Urho3D
{

void Foundation_Texture2DInspector(Context* context, InspectorTab* inspectorTab)
{
    inspectorTab->RegisterAddon<Texture2DInspector>(inspectorTab->GetProject());
}

Texture2DInspector::Texture2DInspector(Project* project)
    : BaseClassName(project)
{
}

StringHash Texture2DInspector::GetResourceType() const
{
    return Texture2D::GetTypeStatic();
}

SharedPtr<BaseWidget> Texture2DInspector::MakePreviewWidget(Resource* resource)
{
    return MakeShared<Texture2DWidget>(context_, static_cast<Texture2D*>(resource));
}

SharedPtr<ResourceInspectorWidget> Texture2DInspector::MakeInspectorWidget(const ResourceVector& resources)
{
    return MakeShared<Texture2DInspectorWidget>(context_, resources);
}

} // namespace Urho3D
