//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include "../Precompiled.h"

#include <EASTL/sort.h>

#include "../Core/Context.h"
#include "../IO/File.h"
#include "../IO/FileSystem.h"
#include "../Input/InputEvents.h"
#include "../UI/DropDownList.h"
#include "../UI/FileSelector.h"
#include "../UI/LineEdit.h"
#include "../UI/ListView.h"
#include "../UI/Text.h"
#include "../UI/UI.h"
#include "../UI/UIEvents.h"
#include "../UI/Window.h"

#include "../DebugNew.h"

namespace Urho3D
{

static bool CompareEntries(const FileSelectorEntry& lhs, const FileSelectorEntry& rhs)
{
    if (lhs.directory_ && !rhs.directory_)
        return true;
    if (!lhs.directory_ && rhs.directory_)
        return false;
    return lhs.name_.comparei(rhs.name_) < 0;
}

FileSelector::FileSelector(Context* context) :
    Object(context),
    directoryMode_(false),
    ignoreEvents_(false)
{
    window_ = context_->CreateObject<Window>();
    window_->SetLayout(LM_VERTICAL);

    titleLayout = context_->CreateObject<UIElement>();
    titleLayout->SetLayout(LM_HORIZONTAL);
    window_->AddChild(titleLayout.Get());

    titleText_ = context_->CreateObject<Text>();
    titleLayout->AddChild(titleText_.Get());

    closeButton_ = context_->CreateObject<Button>();
    titleLayout->AddChild(closeButton_.Get());

    pathEdit_ = context_->CreateObject<LineEdit>();
    window_->AddChild(pathEdit_.Get());

    fileList_ = context_->CreateObject<ListView>();
    window_->AddChild(fileList_.Get());

    fileNameLayout_ = context_->CreateObject<UIElement>();
    fileNameLayout_->SetLayout(LM_HORIZONTAL);

    fileNameEdit_ = context_->CreateObject<LineEdit>();
    fileNameLayout_->AddChild(fileNameEdit_.Get());

    filterList_ = context_->CreateObject<DropDownList>();
    fileNameLayout_->AddChild(filterList_.Get());

    window_->AddChild(fileNameLayout_.Get());

    separatorLayout_ = context_->CreateObject<UIElement>();
    window_->AddChild(separatorLayout_.Get());

    buttonLayout_ = context_->CreateObject<UIElement>();
    buttonLayout_->SetLayout(LM_HORIZONTAL);

    auto spacer = context_->CreateObject<UIElement>();
    buttonLayout_->AddChild(spacer.Get()); // Add spacer

    cancelButton_ = context_->CreateObject<Button>();
    cancelButtonText_ = context_->CreateObject<Text>();
    cancelButtonText_->SetAlignment(HA_CENTER, VA_CENTER);
    cancelButton_->AddChild(cancelButtonText_.Get());
    buttonLayout_->AddChild(cancelButton_.Get());

    okButton_ = context_->CreateObject<Button>();
    okButtonText_ = context_->CreateObject<Text>();
    okButtonText_->SetAlignment(HA_CENTER, VA_CENTER);
    okButton_->AddChild(okButtonText_.Get());
    buttonLayout_->AddChild(okButton_.Get());

    window_->AddChild(buttonLayout_.Get());

    ea::vector<ea::string> defaultFilters;
    defaultFilters.push_back("*.*");
    SetFilters(defaultFilters, 0);
    auto* fileSystem = GetSubsystem<FileSystem>();
    SetPath(fileSystem->GetCurrentDir());

    // Focus the fileselector's filelist initially when created, and bring to front
    auto* ui = GetSubsystem<UI>();
    ui->GetRoot()->AddChild(window_.Get());
    ui->SetFocusElement(fileList_.Get());
    window_->SetModal(true);

    SubscribeToEvent(filterList_, E_ITEMSELECTED, URHO3D_HANDLER(FileSelector, HandleFilterChanged));
    SubscribeToEvent(pathEdit_, E_TEXTFINISHED, URHO3D_HANDLER(FileSelector, HandlePathChanged));
    SubscribeToEvent(fileNameEdit_, E_TEXTFINISHED, URHO3D_HANDLER(FileSelector, HandleOKPressed));
    SubscribeToEvent(fileList_, E_ITEMSELECTED, URHO3D_HANDLER(FileSelector, HandleFileSelected));
    SubscribeToEvent(fileList_, E_ITEMDOUBLECLICKED, URHO3D_HANDLER(FileSelector, HandleFileDoubleClicked));
    SubscribeToEvent(fileList_, E_UNHANDLEDKEY, URHO3D_HANDLER(FileSelector, HandleFileListKey));
    SubscribeToEvent(okButton_, E_RELEASED, URHO3D_HANDLER(FileSelector, HandleOKPressed));
    SubscribeToEvent(cancelButton_, E_RELEASED, URHO3D_HANDLER(FileSelector, HandleCancelPressed));
    SubscribeToEvent(closeButton_, E_RELEASED, URHO3D_HANDLER(FileSelector, HandleCancelPressed));
    SubscribeToEvent(window_, E_MODALCHANGED, URHO3D_HANDLER(FileSelector, HandleCancelPressed));
}

FileSelector::~FileSelector()
{
    window_->Remove();
}

void FileSelector::RegisterObject(Context* context)
{
    context->RegisterFactory<FileSelector>();
}

void FileSelector::SetDefaultStyle(XMLFile* style)
{
    if (!style)
        return;

    window_->SetDefaultStyle(style);
    window_->SetStyle("FileSelector");

    titleText_->SetStyle("FileSelectorTitleText");
    closeButton_->SetStyle("CloseButton");

    okButtonText_->SetStyle("FileSelectorButtonText");
    cancelButtonText_->SetStyle("FileSelectorButtonText");

    titleLayout->SetStyle("FileSelectorLayout");
    fileNameLayout_->SetStyle("FileSelectorLayout");
    buttonLayout_->SetStyle("FileSelectorLayout");
    separatorLayout_->SetStyle("EditorSeparator");

    fileList_->SetStyle("FileSelectorListView");
    fileNameEdit_->SetStyle("FileSelectorLineEdit");
    pathEdit_->SetStyle("FileSelectorLineEdit");

    filterList_->SetStyle("FileSelectorFilterList");

    okButton_->SetStyle("FileSelectorButton");
    cancelButton_->SetStyle("FileSelectorButton");

    const ea::vector<SharedPtr<UIElement> >& filterTexts = filterList_->GetListView()->GetContentElement()->GetChildren();
    for (unsigned i = 0; i < filterTexts.size(); ++i)
        filterTexts[i]->SetStyle("FileSelectorFilterText");

    const ea::vector<SharedPtr<UIElement> >& listTexts = fileList_->GetContentElement()->GetChildren();
    for (unsigned i = 0; i < listTexts.size(); ++i)
        listTexts[i]->SetStyle("FileSelectorListText");

    UpdateElements();
}

void FileSelector::SetTitle(const ea::string& text)
{
    titleText_->SetText(text);
}

void FileSelector::SetButtonTexts(const ea::string& okText, const ea::string& cancelText)
{
    okButtonText_->SetText(okText);
    cancelButtonText_->SetText(cancelText);
}

void FileSelector::SetPath(const ea::string& path)
{
    auto* fileSystem = GetSubsystem<FileSystem>();
    if (fileSystem->DirExists(path))
    {
        path_ = AddTrailingSlash(path);
        SetLineEditText(pathEdit_, path_);
        RefreshFiles();
    }
    else
    {
        // If path was invalid, restore the old path to the line edit
        if (pathEdit_->GetText() != path_)
            SetLineEditText(pathEdit_, path_);
    }
}

void FileSelector::SetFileName(const ea::string& fileName)
{
    SetLineEditText(fileNameEdit_, fileName);
}

void FileSelector::SetFilters(const ea::vector<ea::string>& filters, unsigned defaultIndex)
{
    if (filters.empty())
        return;

    ignoreEvents_ = true;

    filters_ = filters;
    filterList_->RemoveAllItems();
    for (unsigned i = 0; i < filters_.size(); ++i)
    {
        auto filterText = context_->CreateObject<Text>();
        filterList_->AddItem(filterText);
        filterText->SetText(filters_[i]);
        filterText->SetStyle("FileSelectorFilterText");
    }
    if (defaultIndex > filters.size())
        defaultIndex = 0;
    filterList_->SetSelection(defaultIndex);

    ignoreEvents_ = false;

    if (GetFilter() != lastUsedFilter_)
        RefreshFiles();
}

void FileSelector::SetDirectoryMode(bool enable)
{
    directoryMode_ = enable;
}

void FileSelector::UpdateElements()
{
    buttonLayout_->SetFixedHeight(Max(okButton_->GetHeight(), cancelButton_->GetHeight()));
}

XMLFile* FileSelector::GetDefaultStyle() const
{
    return window_->GetDefaultStyle(false);
}

const ea::string& FileSelector::GetTitle() const
{
    return titleText_->GetText();
}

const ea::string& FileSelector::GetFileName() const
{
    return fileNameEdit_->GetText();
}

const ea::string& FileSelector::GetFilter() const
{
    auto* selectedFilter = static_cast<Text*>(filterList_->GetSelectedItem());
    if (selectedFilter)
        return selectedFilter->GetText();
    else
        return EMPTY_STRING;
}

unsigned FileSelector::GetFilterIndex() const
{
    return filterList_->GetSelection();
}

void FileSelector::SetLineEditText(LineEdit* edit, const ea::string& text)
{
    ignoreEvents_ = true;
    edit->SetText(text);
    ignoreEvents_ = false;
}

void FileSelector::RefreshFiles()
{
    auto* fileSystem = GetSubsystem<FileSystem>();

    ignoreEvents_ = true;

    fileList_->RemoveAllItems();
    fileEntries_.clear();

    ea::vector<ea::string> directories;
    ea::vector<ea::string> files;
    fileSystem->ScanDir(directories, path_, "*", SCAN_DIRS, false);
    fileSystem->ScanDir(files, path_, GetFilter(), SCAN_FILES, false);

    fileEntries_.reserve(directories.size() + files.size());

    for (unsigned i = 0; i < directories.size(); ++i)
    {
        FileSelectorEntry newEntry;
        newEntry.name_ = directories[i];
        newEntry.directory_ = true;
        fileEntries_.push_back(newEntry);
    }

    for (unsigned i = 0; i < files.size(); ++i)
    {
        FileSelectorEntry newEntry;
        newEntry.name_ = files[i];
        newEntry.directory_ = false;
        fileEntries_.push_back(newEntry);
    }

    // Sort and add to the list view
    // While items are being added, disable layout update for performance optimization
    ea::quick_sort(fileEntries_.begin(), fileEntries_.end(), CompareEntries);
    UIElement* listContent = fileList_->GetContentElement();
    listContent->DisableLayoutUpdate();
    for (unsigned i = 0; i < fileEntries_.size(); ++i)
    {
        ea::string displayName;
        if (fileEntries_[i].directory_)
            displayName = "<DIR> " + fileEntries_[i].name_;
        else
            displayName = fileEntries_[i].name_;

        auto entryText = context_->CreateObject<Text>();
        fileList_->AddItem(entryText);
        entryText->SetText(displayName);
        entryText->SetStyle("FileSelectorListText");
    }
    listContent->EnableLayoutUpdate();
    listContent->UpdateLayout();

    ignoreEvents_ = false;

    // Clear filename from the previous dir so that there is no confusion
    SetFileName(EMPTY_STRING);
    lastUsedFilter_ = GetFilter();
}

bool FileSelector::EnterFile()
{
    unsigned index = fileList_->GetSelection();
    if (index >= fileEntries_.size())
        return false;

    if (fileEntries_[index].directory_)
    {
        // If a directory double clicked, enter it. Recognize . and .. as a special case
        const ea::string& newPath = fileEntries_[index].name_;
        if ((newPath != ".") && (newPath != ".."))
            SetPath(path_ + newPath);
        else if (newPath == "..")
        {
            ea::string parentPath = GetParentPath(path_);
            SetPath(parentPath);
        }

        return true;
    }
    else
    {
        // Double clicking a file is the same as pressing OK
        if (!directoryMode_)
        {
            using namespace FileSelected;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_FILENAME] = path_ + fileEntries_[index].name_;
            eventData[P_FILTER] = GetFilter();
            eventData[P_OK] = true;
            SendEvent(E_FILESELECTED, eventData);
        }
    }

    return false;
}

void FileSelector::HandleFilterChanged(StringHash eventType, VariantMap& eventData)
{
    if (ignoreEvents_)
        return;

    if (GetFilter() != lastUsedFilter_)
        RefreshFiles();
}

void FileSelector::HandlePathChanged(StringHash eventType, VariantMap& eventData)
{
    if (ignoreEvents_)
        return;

    // Attempt to set path. Restores old if does not exist
    SetPath(pathEdit_->GetText());
}

void FileSelector::HandleFileSelected(StringHash eventType, VariantMap& eventData)
{
    if (ignoreEvents_)
        return;

    unsigned index = fileList_->GetSelection();
    if (index >= fileEntries_.size())
        return;
    // If a file selected, update the filename edit field
    if (!fileEntries_[index].directory_)
        SetFileName(fileEntries_[index].name_);
}

void FileSelector::HandleFileDoubleClicked(StringHash eventType, VariantMap& eventData)
{
    if (ignoreEvents_)
        return;

    if (eventData[ItemDoubleClicked::P_BUTTON] == MOUSEB_LEFT)
        EnterFile();
}

void FileSelector::HandleFileListKey(StringHash eventType, VariantMap& eventData)
{
    if (ignoreEvents_)
        return;

    using namespace UnhandledKey;

    int key = eventData[P_KEY].GetInt();
    if (key == KEY_RETURN || key == KEY_RETURN2 || key == KEY_KP_ENTER)
    {
        bool entered = EnterFile();
        // When a key is used to enter a directory, select the first file if no selection
        if (entered && !fileList_->GetSelectedItem())
            fileList_->SetSelection(0);
    }
}

void FileSelector::HandleOKPressed(StringHash eventType, VariantMap& eventData)
{
    if (ignoreEvents_)
        return;

    const ea::string& fileName = GetFileName();

    if (!directoryMode_)
    {
        if (!fileName.empty())
        {
            using namespace FileSelected;

            VariantMap& newEventData = GetEventDataMap();
            newEventData[P_FILENAME] = path_ + GetFileName();
            newEventData[P_FILTER] = GetFilter();
            newEventData[P_OK] = true;
            SendEvent(E_FILESELECTED, newEventData);
        }
    }
    else if (eventType == E_RELEASED && !path_.empty())
    {
        using namespace FileSelected;

        VariantMap& newEventData = GetEventDataMap();
        newEventData[P_FILENAME] = path_;
        newEventData[P_FILTER] = GetFilter();
        newEventData[P_OK] = true;
        SendEvent(E_FILESELECTED, newEventData);
    }
}

void FileSelector::HandleCancelPressed(StringHash eventType, VariantMap& eventData)
{
    if (ignoreEvents_)
        return;

    if (eventType == E_MODALCHANGED && eventData[ModalChanged::P_MODAL].GetBool())
        return;

    using namespace FileSelected;

    VariantMap& newEventData = GetEventDataMap();
    newEventData[P_FILENAME] = EMPTY_STRING;
    newEventData[P_FILTER] = GetFilter();
    newEventData[P_OK] = false;
    SendEvent(E_FILESELECTED, newEventData);
}

}
