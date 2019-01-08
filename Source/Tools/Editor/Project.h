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

#pragma once


#include <Urho3D/Core/Object.h>
#if URHO3D_PLUGINS
#   include "Plugins/PluginManager.h"
#endif
#include "Pipeline/Pipeline.h"


namespace Urho3D
{

class Project : public Object
{
    URHO3D_OBJECT(Project, Object);
public:
    /// Construct.
    explicit Project(Context* context);
    /// Destruct.
    ~Project() override;
    /// Load existing project. Returns true if project was successfully loaded.
    bool LoadProject(const String& projectPath);
    /// Create a new project. Returns true if successful. Overwrites specified path unconditionally.
    bool SaveProject();
    /// Return project directory.
    const String& GetProjectPath() const { return projectFileDir_; }
    /// Returns path to temporary asset cache.
    String GetCachePath() const;
    /// Returns path to permanent asset cache.
    String GetResourcePath() const;
#if URHO3D_PLUGINS
    /// Returns plugin manager.
    PluginManager* GetPlugins() { return &plugins_; }
#endif
    /// Returns true in very first session of new project.
    bool IsNewProject() const { return isNewProject_; }
    /// Return resource name of scene that will be executed first by the player.
    const String& GetDefaultSceneName() const { return defaultScene_; }
    /// Set resource name of scene that will be executed first by the player.
    void SetDefaultSceneName(const String& defaultScene) { defaultScene_ = defaultScene; }
    /// Returns a map of default engine settings that will be applied on the start of player application.
    HashMap<String, Variant>& GetDefaultEngineSettings() { return engineParameters_; }
    ///
    Pipeline& GetPipeline() { return pipeline_; }

protected:
    /// Directory containing project.
    String projectFileDir_;
    ///
    Pipeline pipeline_;
    /// Copy of engine resource paths that get unregistered when project is loaded.
    StringVector cachedEngineResourcePaths_;
    /// Path to imgui settings ini file.
    String uiConfigPath_;
#if URHO3D_PLUGINS
    /// Native plugin manager.
    PluginManager plugins_;
#endif
    /// Flag indicating that project was just created.
    bool isNewProject_ = true;
    /// Resource name of scene that will be started by player first.
    String defaultScene_;
    ///
    HashMap<String, Variant> engineParameters_;
    ///
    Timer saveProjectTimer_;
};


}
