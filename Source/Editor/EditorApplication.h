// Copyright (c) 2017-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Core/EditorPluginManager.h"
#include "Project/Project.h"

#include <Urho3D/Engine/Application.h>

namespace Urho3D
{

class EditorApplication : public Application
{
    URHO3D_OBJECT(EditorApplication, Application);

public:
    explicit EditorApplication(Context* context);
    ~EditorApplication() override = default;

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
    /// Whether the editor is launched in single process mode.
    bool singleProcess_{};
    /// Launch command and command line parameters.
    ea::string command_;
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

} // namespace Urho3D
