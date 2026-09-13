// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Project/ResourceFactory.h"

#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/SystemUI/SystemUI.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

#include <EASTL/tuple.h>

namespace Urho3D
{

ResourceFactory::ResourceFactory(Context* context, int group, const ea::string& title)
    : Object(context)
    , group_(group)
    , title_(title)
{
}

bool ResourceFactory::Compare(
    const SharedPtr<ResourceFactory>& lhs, const SharedPtr<ResourceFactory>& rhs)
{
    return ea::tie(lhs->group_, lhs->title_) < ea::tie(rhs->group_, rhs->title_);
}

BaseResourceFactory::BaseResourceFactory(Context* context, int group, const ea::string& title)
    : ResourceFactory(context, group, title)
{
}

void BaseResourceFactory::Open(const ea::string& baseFilePath, const ea::string baseResourcePath)
{
    baseFilePath_ = AddTrailingSlash(baseFilePath);
    baseResourcePath_ = AddTrailingSlash(baseResourcePath);
    localFileName_ = GetDefaultFileName();

    selectFileNameInput_ = IsFileNameEditable();
}

void BaseResourceFactory::Render(const FileNameChecker& checker, bool& canCommit, bool& shouldCommit)
{
    const auto [isFileNameValid, extraLine] = checker(baseFilePath_, localFileName_);
    const ea::string fileName = baseFilePath_ + localFileName_;
    const ea::string resourceName = baseResourcePath_ + localFileName_;
    ui::Text("Would you like to create '%s'?\n%s", resourceName.c_str(), extraLine.c_str());

    if (selectFileNameInput_)
        ui::SetKeyboardFocusHere();
    ui::BeginDisabled(!IsFileNameEditable());
    const bool isEnterPressed = ui::InputText("##FileName", &localFileName_,
        ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
    ui::EndDisabled();

    newResourcePath_ = RemoveTrailingSlash(baseResourcePath_);
    ui::Text(ICON_FA_FOLDER " in folder:");
    if (ui::InputText("##ResourcePath", &newResourcePath_))
    {
        URHO3D_ASSERT(baseFilePath_.ends_with(baseResourcePath_));
        baseFilePath_.erase(baseFilePath_.size() - baseResourcePath_.size());
        baseResourcePath_ = AddTrailingSlash(newResourcePath_);
        baseFilePath_ += baseResourcePath_;
    }

    RenderAuxilary();

    selectFileNameInput_ = false;

    canCommit = isFileNameValid;
    shouldCommit = isEnterPressed;
}

void BaseResourceFactory::CommitAndClose()
{
}

ea::string BaseResourceFactory::GetFinalFileName() const
{
    return baseFilePath_ + localFileName_;
}

ea::string BaseResourceFactory::GetFinalResourceName() const
{
    return baseResourcePath_ + localFileName_;
}

SimpleResourceFactory::SimpleResourceFactory(Context* context,
    int group, const ea::string& title, const ea::string& fileName, const Callback& callback)
    : BaseResourceFactory(context, group, title)
    , fileName_(fileName)
    , callback_(callback)
{
    URHO3D_ASSERT(callback_);
}

void SimpleResourceFactory::CommitAndClose()
{
    callback_(GetFinalFileName(), GetFinalResourceName());
}

}
