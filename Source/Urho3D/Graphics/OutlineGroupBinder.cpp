//
// Copyright (c) 2022-2024 the rbfx project.
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

#include "../Graphics/OutlineGroupBinder.h"
#include "../Graphics/OutlineGroup.h"
#include "../Graphics/Drawable.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

OutlineGroupBinder::OutlineGroupBinder(Context* context)
    : BaseClassName(context)
{
}

OutlineGroupBinder::~OutlineGroupBinder()
{
    Unbind();
}

void OutlineGroupBinder::Unbind()
{
    if (!drawable_.Expired() && !outlineGroup_.Expired() && outlineGroup_->HasDrawable(drawable_))
    {
        outlineGroup_->RemoveDrawable(drawable_);
    }

    drawable_.Reset();
    outlineGroup_.Reset();
}

void OutlineGroupBinder::Bind(Scene* scene)
{
    Unbind();

    if (!scene)
    {
        return;
    }

    auto drawable = node_->GetDerivedComponent<Drawable>();
    if (!drawable)
    {
        URHO3D_LOGWARNING("OutlineGroupBinder on node {} doesn't have a drawable", node_->GetName());
        return;
    }

    ea::vector<OutlineGroup*> outlineGroups;
    scene->GetComponents(outlineGroups);
    auto outlineGroupItr =
        ea::find_if(outlineGroups.begin(), outlineGroups.end(), [](const auto* group) { return !group->IsDebug(); });
    if (outlineGroupItr == outlineGroups.end())
    {
        URHO3D_LOGWARNING(
            "OutlineGroupBinder on node {} is in a scene that doesn't have a non-debug OutlineGroup in it",
            node_->GetName());
        return;
    }

    drawable_.Reset(drawable);
    outlineGroup_.Reset(*outlineGroupItr);

    if (!outlineGroup_->HasDrawable(drawable))
    {
        outlineGroup_->AddDrawable(drawable_);
    }
}

void OutlineGroupBinder::OnSetEnabled()
{
    if (enabled_)
    {
        Bind(GetScene());
    }
    else
    {
        Unbind();
    }
}

void OutlineGroupBinder::OnSceneSet(Scene* scene)
{
    Bind(scene);
}

void OutlineGroupBinder::RegisterObject(Context* context)
{
    context->AddFactoryReflection<OutlineGroupBinder>(Category_Scene);
}

}
