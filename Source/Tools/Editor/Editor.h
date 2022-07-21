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

#include "Core/EditorPluginManager.h"
#include "Project/ProjectEditor.h"

#include <Urho3D/Engine/Application.h>

namespace Urho3D
{

class Editor : public Application
{
    URHO3D_OBJECT(Editor, Application);
public:
    /// Construct.
    explicit Editor(Context* context);
    /// Destruct.
    ~Editor() override = default;
    /// Set up editor application.
    void Setup() override;
    /// Initialize editor application.
    void Start() override;
    /// Tear down editor application.
    void Stop() override;

    /// Renders UI elements.
    void OnUpdate(VariantMap& args);
    /// Renders menu bar at the top of the screen.
    void RenderMenuBar();

    /// Opens project or creates new one.
    void OpenProject(const ea::string& projectPath);
    /// Close current project.
    void CloseProject();
    /// Return path containing data directories of engine.
    const ea::string& GetCoreResourcePrefixPath() const { return coreResourcePrefixPath_; }
    ///
    ImFont* GetMonoSpaceFont() const { return monoFont_; }
    ///
    void UpdateWindowTitle(const ea::string& resourcePath=EMPTY_STRING);
    ///
    StringVariantMap& GetEngineParameters() { return engineParameters_; }
    ProjectEditor* GetProjectEditor() const { return projectEditor_; }

protected:
    /// Process console commands.
    void OnConsoleCommand(VariantMap& args);
    /// Housekeeping tasks.
    void OnEndFrame();
    /// Handle user closing editor window.
    void OnExitRequested();
    /// Handle user closing editor with a hotkey.
    void OnExitHotkeyPressed();
    ///
    void SetupSystemUI();
    /// Opens a file dialog for folder selection. Project is opened or created in selected folder.
    void OpenOrCreateProject();
    ///
    void OnConsoleUriClick(VariantMap& args);

    /// Prefix path of CoreData and EditorData.
    ea::string coreResourcePrefixPath_;
    /// Editor plugins.
    SharedPtr<EditorPluginManager> editorPluginManager_;
    /// Currently loaded project.
    SharedPtr<ProjectEditor> projectEditor_;
    /// Path to a project that editor should open on the end of the frame.
    ea::string pendingOpenProject_;
    bool pendingCloseProject_{};
    /// Monospace font.
    ImFont* monoFont_ = nullptr;
    /// Flag indicating editor is exiting.
    bool exiting_ = false;
    /// Project path passed on command line.
    ea::string defaultProjectPath_;
    /// A list of of recently opened projects. First one is alaways last project that was opened.
    StringVector recentProjects_{};
#if 0
    /// Window position which is saved between sessions.
    IntVector2 windowPos_{0, 0};
    /// Window size which is saved between sessions.
    IntVector2 windowSize_{1920, 1080};
#endif
};

}
