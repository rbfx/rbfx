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
    if (!outlineGroups_.Expired())
    {
        for (auto& drawable : drawables_)
        {
            if (!drawable.Expired())
            {
                outlineGroups_->RemoveDrawable(drawable);
            }
        }
    }

    drawables_.clear();
    outlineGroups_.Reset();
}

void OutlineGroupBinder::Bind(Scene* scene)
{
    Unbind();

    if (!scene)
    {
        return;
    }

    if (binderTag_.empty())
    {
        return;
    }

    ea::vector<OutlineGroup*> outlineGroups;
    scene->GetComponents(outlineGroups);
    auto outlineGroupItr =
        ea::find_if(outlineGroups.begin(), outlineGroups.end(), [this](const auto* group) { return group->GetBinderTag() == binderTag_; });
    if (outlineGroupItr == outlineGroups.end())
    {
        URHO3D_LOGWARNING(
            "OutlineGroupBinder on node {} is in a scene that doesn't have an OutlineGroup with the binder tag of '{}'",
            node_->GetName(),
            binderTag_);
        return;
    }

    ea::vector<Node*> nodes;
    nodes.push_back(node_);

    if (recursive_)
    {
        nodes.append(node_->GetChildren(true));
    }

    for (auto node : nodes)
    {
        auto& components = node->GetComponents();
        for (auto component : components)
        {
            if (component->IsInstanceOf(Drawable::GetTypeStatic()))
            {
                auto drawable = static_cast<Drawable*>(component.GetPointer());
                drawables_.push_back(WeakPtr(drawable));
            }
        }
    }

    if (drawables_.empty())
    {
        URHO3D_LOGWARNING("OutlineGroupBinder on node {} doesn't have any drawables on it (recursive={})", node_->GetName(), recursive_);
        return;
    }

    outlineGroups_.Reset(*outlineGroupItr);

    for (auto drawable : drawables_)
    {
        if (!outlineGroups_->HasDrawable(drawable))
        {
            outlineGroups_->AddDrawable(drawable);
        }
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

    URHO3D_ACCESSOR_ATTRIBUTE("Is Recursive", IsRecursive, SetRecursive, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Binder Tag", GetBinderTag, SetBinderTag, ea::string, EMPTY_STRING, AM_DEFAULT);
}

void OutlineGroupBinder::SetBinderTag(const ea::string& tag)
{
    binderTag_ = tag;
    if (enabled_)
    {
        Bind(GetScene());
    }
}

}
