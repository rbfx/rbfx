//
// Copyright (c) 2008-2022 the Urho3D project.
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

#pragma once

#include "../Core/Object.h"

namespace Urho3D
{

class Button;
class DropDownList;
class Font;
class LineEdit;
class ListView;
class ResourceCache;
class Text;
class UIElement;
class Window;
class XMLFile;

/// %File selector's list entry (file or directory).
struct FileSelectorEntry
{
    /// Name.
    ea::string name_;
    /// Directory flag.
    bool directory_;
};

/// %File selector dialog.
class URHO3D_API FileSelector : public Object
{
    URHO3D_OBJECT(FileSelector, Object);

public:
    /// Construct.
    explicit FileSelector(Context* context);
    /// Destruct.
    ~FileSelector() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Set fileselector UI style.
    /// @property
    void SetDefaultStyle(XMLFile* style);
    /// Set title text.
    /// @property
    void SetTitle(const ea::string& text);
    /// Set button texts.
    void SetButtonTexts(const ea::string& okText, const ea::string& cancelText);
    /// Set current path.
    /// @property
    void SetPath(const ea::string& path);
    /// Set current filename.
    /// @property
    void SetFileName(const ea::string& fileName);
    /// Set filters.
    void SetFilters(const ea::vector<ea::string>& filters, unsigned defaultIndex);
    /// Set directory selection mode. Default false.
    /// @property
    void SetDirectoryMode(bool enable);
    /// Update elements to layout properly. Call this after manually adjusting the sub-elements.
    void UpdateElements();

    /// Return the UI style file.
    /// @property
    XMLFile* GetDefaultStyle() const;

    /// Return fileselector window.
    /// @property
    Window* GetWindow() const { return window_; }

    /// Return window title text element.
    /// @property
    Text* GetTitleText() const { return titleText_; }

    /// Return file list.
    /// @property
    ListView* GetFileList() const { return fileList_; }

    /// Return path editor.
    /// @property
    LineEdit* GetPathEdit() const { return pathEdit_; }

    /// Return filename editor.
    /// @property
    LineEdit* GetFileNameEdit() const { return fileNameEdit_; }

    /// Return filter dropdown.
    /// @property
    DropDownList* GetFilterList() const { return filterList_; }

    /// Return OK button.
    /// @property{get_okButton}
    Button* GetOKButton() const { return okButton_; }

    /// Return cancel button.
    /// @property
    Button* GetCancelButton() const { return cancelButton_; }

    /// Return close button.
    Button* GetCloseButton() const { return closeButton_; }

    /// Return window title.
    /// @property
    const ea::string& GetTitle() const;

    /// Return current path.
    /// @property
    const ea::string& GetPath() const { return path_; }

    /// Return current filename.
    /// @property
    const ea::string& GetFileName() const;
    /// Return current filter.
    /// @property
    const ea::string& GetFilter() const;
    /// Return current filter index.
    /// @property
    unsigned GetFilterIndex() const;

    /// Return directory mode flag.
    /// @property
    bool GetDirectoryMode() const { return directoryMode_; }

private:
    /// Set the text of an edit field and ignore the resulting event.
    void SetLineEditText(LineEdit* edit, const ea::string& text);
    /// Refresh the directory listing.
    void RefreshFiles();
    /// Enter a directory or confirm a file. Return true if a directory entered.
    bool EnterFile();
    /// Handle filter changed.
    void HandleFilterChanged(StringHash eventType, VariantMap& eventData);
    /// Handle path edited.
    void HandlePathChanged(StringHash eventType, VariantMap& eventData);
    /// Handle file selected from the list.
    void HandleFileSelected(StringHash eventType, VariantMap& eventData);
    /// Handle file doubleclicked from the list (enter directory / OK the file selection).
    void HandleFileDoubleClicked(StringHash eventType, VariantMap& eventData);
    /// Handle file list key pressed.
    void HandleFileListKey(StringHash eventType, VariantMap& eventData);
    /// Handle OK button pressed.
    void HandleOKPressed(StringHash eventType, VariantMap& eventData);
    /// Handle cancel button pressed.
    void HandleCancelPressed(StringHash eventType, VariantMap& eventData);

    /// Fileselector window.
    SharedPtr<Window> window_;
    /// Title layout.
    SharedPtr<UIElement> titleLayout;
    /// Window title text.
    SharedPtr<Text> titleText_;
    /// File list.
    SharedPtr<ListView> fileList_;
    /// Path editor.
    SharedPtr<LineEdit> pathEdit_;
    /// Filename editor.
    SharedPtr<LineEdit> fileNameEdit_;
    /// Filter dropdown.
    SharedPtr<DropDownList> filterList_;
    /// OK button.
    SharedPtr<Button> okButton_;
    /// OK button text.
    SharedPtr<Text> okButtonText_;
    /// Cancel button.
    SharedPtr<Button> cancelButton_;
    /// Cancel button text.
    SharedPtr<Text> cancelButtonText_;
    /// Close button.
    SharedPtr<Button> closeButton_;
    /// Filename and filter layout.
    SharedPtr<UIElement> fileNameLayout_;
    /// Separator layout.
    SharedPtr<UIElement> separatorLayout_;
    /// Button layout.
    SharedPtr<UIElement> buttonLayout_;
    /// Current directory.
    ea::string path_;
    /// Filters.
    ea::vector<ea::string> filters_;
    /// File entries.
    ea::vector<FileSelectorEntry> fileEntries_;
    /// Filter used to get the file list.
    ea::string lastUsedFilter_;
    /// Directory mode flag.
    bool directoryMode_;
    /// Ignore events flag, used when changing line edits manually.
    bool ignoreEvents_;
};

}
