//
// Copyright (c) 2018 Rokas Kupstys
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
#include <Urho3D/Urho3DAll.h>
#include "BaseResourceTab.h"

namespace Urho3D
{

BaseResourceTab::BaseResourceTab(Context* context)
    : Tab(context)
    , undo_(context)
{
    SubscribeToEvent(E_RESOURCERENAMED, [&](StringHash, VariantMap& args) {
        using namespace ResourceRenamed;

        auto nameFrom = args[P_FROM].GetString();
        if (resourceName_ == nameFrom)
            SetResourceName(args[P_TO].GetString());
    });
}

bool BaseResourceTab::LoadResource(const Urho3D::String& resourcePath)
{
    if (!Tab::LoadResource(resourcePath))
        return false;

    if (resourcePath.Empty())
        return false;

    SetResourceName(resourcePath);
    undo_.Clear();
    lastUndoIndex_ = undo_.Index();
    return true;
}

bool Urho3D::BaseResourceTab::SaveResource()
{
    if (!Tab::SaveResource())
        return false;

    if (resourceName_.Empty())
        return false;

    lastUndoIndex_ = undo_.Index();

    return true;
}

void BaseResourceTab::OnSaveProject(JSONValue& tab)
{
    Tab::OnSaveProject(tab);
    tab["path"] = resourceName_;
}

void BaseResourceTab::OnLoadProject(const JSONValue& tab)
{
    Tab::OnLoadProject(tab);
    LoadResource(tab["path"].GetString());
}

void BaseResourceTab::SetResourceName(const String& resourceName)
{
    resourceName_ = resourceName;
    if (!isUtility_)
        SetTitle(GetFileName(resourceName_));
}

bool BaseResourceTab::IsModified() const
{
    return lastUndoIndex_ != undo_.Index();
}

}
