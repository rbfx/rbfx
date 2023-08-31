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
#include "Project/Project.h"

#include <Urho3D/Engine/Application.h>

namespace Urho3D
{

class Editor : public Application
{
    URHO3D_OBJECT(Editor, Application);

public:
    explicit Editor(Context* context);
    ~Editor() override = default;

    /// Implement Application
    /// @{
    void SerializeInBlock(Archive& archive) override;
    void Setup() override;
    void Start() override;
    void Stop() override;
    /// @}

    /// Opens project or creates new one.
    void OpenProject(const ea::string& projectPath);
    /// Close current project.
    void CloseProject();

protected:
    void InitializeUI();
    void RecreateSystemUI();
    void InitializeSystemUI();
    void InitializeImGuiConfig();
    void InitializeImGuiStyle();
    void InitializeImGuiHandlers();

    Texture2D* GetProjectPreview(const ea::string& projectPath);
    ea::string GetWindowTitle() const;

    void Render();
    void RenderMenuBar();
    void RenderAboutDialog();

    void UpdateProjectStatus();
    void SaveTempJson();
    void OnExitRequested();
    void OnConsoleUriClick(VariantMap& args);

    void OpenOrCreateProject();

    /// Editor paths
    /// @{
    ea::string resourcePrefixPath_;
    ea::string tempJsonPath_;
    ea::string settingsJsonPath_;
    /// @}

    /// Persistent state
    /// @{
    StringVector recentProjects_;
    /// @}

    /// Editor plugins.
    SharedPtr<EditorPluginManager> editorPluginManager_;
    /// Currently loaded project.
    SharedPtr<Project> project_;

    /// Whether the editor is launched in read-only mode.
    bool readOnly_{};
    /// Launch command and command line parameters.
    ea::string command_;
    /// Implicit plugin dynamic library name.
    ea::string implicitPlugin_;
    /// Whether to exit the editor after executing the command.
    bool exitAfterCommand_{};

    /// UI state
    /// @{
    ea::string pendingOpenProject_;
    bool pendingCloseProject_{};
    bool exiting_{};

    bool uiAlreadyInitialized_{};
    ea::string windowTitle_;
    ea::unordered_map<ea::string, SharedPtr<Texture2D>> projectPreviews_;

    bool showAbout_{};

    ea::optional<unsigned> numIncompleteTasks_;
    /// @}
};

}
