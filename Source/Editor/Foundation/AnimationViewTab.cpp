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

#include "../Core/CommonEditorActions.h"
#include "../Core/IniHelpers.h"
#include "../Foundation/AnimationViewTab.h"

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/SystemUI/Widgets.h>

namespace Urho3D
{

namespace
{
}

void Foundation_AnimationViewTab(Context* context, Project* project)
{
    project->AddTab(MakeShared<AnimationViewTab>(context));
}

AnimationViewTab::AnimationViewTab(Context* context)
    : CustomSceneViewTab(context, "Animation", "a8e49ac3-8edb-493c-ac7e-0d42530c62fb",
        EditorTabFlag::NoContentPadding | EditorTabFlag::OpenByDefault, EditorTabPlacement::DockCenter)
{
}

AnimationViewTab::~AnimationViewTab()
{
}

bool AnimationViewTab::CanOpenResource(const ResourceFileDescriptor& desc)
{
    return desc.HasObjectType<Animation>();
}

void AnimationViewTab::RenderContent()
{
    if (!animation_)
        return;

    CustomSceneViewTab::RenderContent();
}

void AnimationViewTab::OnResourceLoaded(const ea::string& resourceName)
{
    auto cache = GetSubsystem<ResourceCache>();
    animation_ = cache->GetResource<Animation>(resourceName);
}

void AnimationViewTab::OnResourceUnloaded(const ea::string& resourceName)
{
    animation_.Reset();
}

void AnimationViewTab::OnActiveResourceChanged(const ea::string& oldResourceName, const ea::string& newResourceName)
{
}

void AnimationViewTab::OnResourceSaved(const ea::string& resourceName)
{
}

void AnimationViewTab::OnResourceShallowSaved(const ea::string& resourceName)
{
}

} // namespace Urho3D
