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


#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Timer.h>


namespace Urho3D
{

class UndoStack;
class Pipeline;
class PluginManager;

class Project : public Object
{
    URHO3D_OBJECT(Project, Object);
public:
    /// Construct.
    explicit Project(Context* context);
    /// Destruct.
    ~Project() override;
    /// Load existing project. Returns true if project was successfully loaded.
    bool LoadProject(const ea::string& projectPath);
    /// Create a new project. Returns true if successful. Overwrites specified path unconditionally.
    bool SaveProject();
    /// Serialize project.
    bool Serialize(Archive& archive) override;
    /// Return project directory.
    const ea::string& GetProjectPath() const { return projectFileDir_; }
    /// Returns path to temporary asset cache.
    const ea::string& GetCachePath() const { return cachePath_; }
    /// Returns path to permanent asset cache.
    const ea::string& GetResourcePath() const { return defaultResourcePath_; }
    /// Return a list of resource directory names relative to project path.
    const StringVector& GetResourcePaths() const { return resourcePaths_; }
#if URHO3D_PLUGINS
    /// Returns plugin manager.
    PluginManager* GetPlugins() { return plugins_; }
#endif
    /// Returns true in very first session of new project.
    bool NeeDefaultUIPlacement() const { return defaultUiPlacement_; }
    /// Return resource name of scene that will be executed first by the player.
    const ea::string& GetDefaultSceneName() const { return defaultScene_; }
    /// Set resource name of scene that will be executed first by the player.
    void SetDefaultSceneName(const ea::string& defaultScene) { defaultScene_ = defaultScene; }

protected:
    /// Save project when resource is saved.
    void OnEditorResourceSaved(StringHash, VariantMap&);
    /// Update default scene if it gets renamed.
    void OnResourceRenamed(StringHash, VariantMap& args);
    /// Update default scene if it gets removed.
    void OnResourceBrowserDelete(StringHash, VariantMap& args);
    /// Auto-save project.
    void OnEndFrame(StringHash, VariantMap&);
    /// Render a project tab in settings window.
    void RenderSettingsUI();
    /// User executed undo action.
    void OnUndo();
    /// User executed redo action.
    void OnRedo();

    /// Directory containing project.
    ea::string projectFileDir_;
    /// Full path of CoreData resource directory. Can be empty.
    ea::string coreDataPath_;
    ///
    SharedPtr<Pipeline> pipeline_;
    ///
    StringVector resourcePaths_;
    /// Absolute path to resource cache directory. Usually projectDir/Cache.
    ea::string cachePath_;
    /// Absolute path to default resource directory. Must always be projectDir/resourcePaths_[0].
    ea::string defaultResourcePath_;
    /// Path to imgui settings ini file.
    ea::string uiConfigPath_;
#if URHO3D_PLUGINS
    /// Native plugin manager.
    SharedPtr<PluginManager> plugins_;
#endif
    /// Flag indicating that project was just created.
    bool defaultUiPlacement_ = true;
    /// Resource name of scene that will be started by player first.
    ea::string defaultScene_;
    /// Timer for project auto-save.
    Timer saveProjectTimer_;
    /// Global undo stack.
    SharedPtr<UndoStack> undo_;
};


}
