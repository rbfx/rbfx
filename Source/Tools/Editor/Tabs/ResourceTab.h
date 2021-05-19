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

#pragma once


#include <EASTL/utility.h>

#include "Tabs/Tab.h"


namespace Urho3D
{

class Asset;
class FileChange;

struct ResourceContextMenuArgs
{
    ea::string resourceName_;
};

URHO3D_EVENT(E_RESOURCEBROWSERDELETE, ResourceBrowserDelete)
{
    URHO3D_PARAM(P_NAME, Name);                                     // String
}

/// Resource browser tab.
class ResourceTab : public Tab
{
    URHO3D_OBJECT(ResourceTab, Tab)
public:
    /// Construct.
    explicit ResourceTab(Context* context);

    /// Render content of tab window. Returns false if tab was closed.
    bool RenderWindowContent() override;
    /// Clear any user selection tracked by this tab.
    void ClearSelection() override;
    /// Serialize current user selection into a buffer and return it.
    bool SerializeSelection(Archive& archive) override;

    /// Signal set when user right-clicks a resource or folder.
    Signal<void(const ResourceContextMenuArgs&)> resourceContextMenu_;

protected:
    ///
    void OnLocateResource(StringHash, VariantMap& args);
    /// Constructs a name for newly created resource based on specified template name.
    ea::string GetNewResourcePath(const ea::string& name);
    /// Select current item in attribute inspector.
    void SelectCurrentItemInspector();
    ///
    void OnEndFrame(StringHash, VariantMap&);
    ///
    void OnResourceUpdated(const FileChange& change);
    ///
    void ScanAssets();
    ///
    void OpenResource(const ea::string& resourceName);
    ///
    void RenderContextMenu();
    ///
    void RenderDirectoryTree(const eastl::string& path = "");
    ///
    void ScanDirTree(StringVector& result, const eastl::string& path);
    ///
    void RenderDeletionDialog();
    ///
    void StartRename();
    ///
    bool RenderRenameWidget(const ea::string& icon = "");

    /// Current open resource path.
    ea::string currentDir_;
    /// Current selected resource file name.
    ea::string selectedItem_;
    /// Assets visible in resource browser.
    ea::vector<WeakPtr<Asset>> assets_;
    /// Cache of directory names at current selected resource path.
    StringVector currentDirs_;
    /// Cache of file names at current selected resource path.
    StringVector currentFiles_;
    /// Flag which requests rescan of current selected resource path.
    bool rescan_ = true;
    /// Flag requesting to scroll to selection on next frame.
    bool scrollToCurrent_ = false;
    /// State cache.
    ValueCache cache_{context_};
    /// Frame at which rename started. Non-zero means rename is in-progress.
    unsigned isRenamingFrame_ = 0;
    /// Current value of text widget during rename.
    ea::string renameBuffer_;
};

}
