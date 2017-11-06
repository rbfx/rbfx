//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include "Editor.h"
#include "EditorEvents.h"
#include "SceneView.h"
#include <ImGui/imgui_internal.h>
#include <IconFontCppHeaders/IconsFontAwesome.h>
#include <Toolbox/Toolbox.h>
#include <Toolbox/SystemUI/ImGuiDock.h>
#include <tinyfiledialogs/tinyfiledialogs.h>
#include <Toolbox/SystemUI/ResourceBrowser.h>
#include <Toolbox/IO/ContentUtilities.h>


URHO3D_DEFINE_APPLICATION_MAIN(Editor);


namespace Urho3D
{

Editor::Editor(Context* context)
    : Application(context)
{
}

void Editor::Setup()
{
    engineParameters_[EP_WINDOW_TITLE] = GetTypeName();
    engineParameters_[EP_HEADLESS] = false;
    engineParameters_[EP_RESOURCE_PREFIX_PATHS] =
        context_->GetFileSystem()->GetProgramDir() + ";;..;../share/Urho3D/Resources";
    engineParameters_[EP_FULL_SCREEN] = false;
    engineParameters_[EP_WINDOW_HEIGHT] = 1080;
    engineParameters_[EP_WINDOW_WIDTH] = 1920;
    engineParameters_[EP_LOG_LEVEL] = LOG_DEBUG;
    engineParameters_[EP_WINDOW_RESIZABLE] = true;
}

void Editor::Start()
{
    GetInput()->SetMouseMode(MM_ABSOLUTE);
    GetInput()->SetMouseVisible(true);
    RegisterToolboxTypes(context_);
    context_->RegisterFactory<Editor>();
    context_->RegisterSubsystem(this);

    GetSystemUI()->ApplyStyleDefault(true, 1.0f);
    GetSystemUI()->AddFont("Fonts/fontawesome-webfont.ttf", 0, {ICON_MIN_FA, ICON_MAX_FA, 0}, true);
    ui::GetStyle().WindowRounding = 3;
    // Disable imgui saving ui settings on it's own. These should be serialized to project file.
    ui::GetIO().IniFilename = 0;

    GetCache()->SetAutoReloadResources(true);

    SubscribeToEvent(E_UPDATE, std::bind(&Editor::OnUpdate, this, _2));

    LoadProject("Etc/DefaultEditorProject.xml");
    // Prevent overwriting example scene.
    sceneViews_.Front()->ClearCachedPaths();
}

void Editor::Stop()
{
    SaveProject(projectFilePath_);
    ui::ShutdownDock();
}

void Editor::SaveProject(const String& filePath)
{
    if (filePath.Empty())
        return;

    SharedPtr<XMLFile> xml(new XMLFile(context_));
    XMLElement root = xml->CreateRoot("project");
    root.SetAttribute("version", "0");

    auto window = root.CreateChild("window");
    window.SetAttribute("width", ToString("%d", GetGraphics()->GetWidth()));
    window.SetAttribute("height", ToString("%d", GetGraphics()->GetHeight()));
    window.SetAttribute("x", ToString("%d", GetGraphics()->GetWindowPosition().x_));
    window.SetAttribute("y", ToString("%d", GetGraphics()->GetWindowPosition().y_));

    auto scenes = root.CreateChild("scenes");
    for (auto& sceneView: sceneViews_)
        sceneView->SaveProject(scenes.CreateChild("scene"));

    ui::SaveDock(root.CreateChild("docks"));

    if (!xml->SaveFile(filePath))
        URHO3D_LOGERRORF("Saving project to %s failed", filePath.CString());
}

void Editor::LoadProject(const String& filePath)
{
    if (filePath.Empty())
        return;

    SharedPtr<XMLFile> xml;
    if (!IsAbsolutePath(filePath))
        xml = GetCache()->GetResource<XMLFile>(filePath);

    if (xml.Null())
    {
        xml = new XMLFile(context_);
        if (!xml->LoadFile(filePath))
            return;
    }

    auto root = xml->GetRoot();
    if (root.NotNull())
    {
        idPool_.Clear();
        auto window = root.GetChild("window");
        if (window.NotNull())
        {
            GetGraphics()->SetMode(ToInt(window.GetAttribute("width")), ToInt(window.GetAttribute("height")));
            GetGraphics()->SetWindowPosition(ToInt(window.GetAttribute("x")), ToInt(window.GetAttribute("y")));
        }

        auto scenes = root.GetChild("scenes");
        sceneViews_.Clear();
        if (scenes.NotNull())
        {
            auto scene = scenes.GetChild("scene");
            while (scene.NotNull())
            {
                CreateNewScene(scene);
                scene = scene.GetNext("scene");
            }
        }

        ui::LoadDock(root.GetChild("docks"));
    }
}

void Editor::OnUpdate(VariantMap& args)
{
    ui::RootDock({0, 20}, ui::GetIO().DisplaySize - ImVec2(0, 20));

    RenderMenuBar();

    ui::SetNextDockPos(nullptr, ui::Slot_Left, ImGuiCond_FirstUseEver);
    if (ui::BeginDock("Hierarchy"))
    {
        if (!activeView_.Expired())
            activeView_->RenderSceneNodeTree();
    }
    ui::EndDock();

    bool renderedWasActive = false;
    for (auto it = sceneViews_.Begin(); it != sceneViews_.End();)
    {
        auto& view = *it;
        if (view->RenderWindow())
        {
            if (view->IsRendered())
            {
                // Only active window may override another active window
                if (renderedWasActive && view->IsActive())
                    activeView_ = view;
                else if (!renderedWasActive)
                {
                    renderedWasActive = view->IsActive();
                    activeView_ = view;
                }
            }

            ++it;
        }
        else
            it = sceneViews_.Erase(it);
    }


    if (!activeView_.Expired())
        ui::SetNextDockPos(activeView_->GetUniqueTitle().CString(), ui::Slot_Right, ImGuiCond_FirstUseEver);
    if (ui::BeginDock("Inspector"))
    {
        if (!activeView_.Expired())
            activeView_->RenderInspector();
    }
    ui::EndDock();

    String selected;
    if (sceneViews_.Size())
        ui::SetNextDockPos(sceneViews_.Back()->GetUniqueTitle().CString(), ui::Slot_Bottom, ImGuiCond_FirstUseEver);
    if (ResourceBrowserWindow(selected, &resourceBrowserWindowOpen_))
    {
        auto type = GetContentType(selected);
        if (type == CTYPE_SCENE)
        {
            CreateNewScene()->LoadScene(selected);
        }
    }
}

void Editor::RenderMenuBar()
{
    bool save = false;
    if (ui::BeginMainMenuBar())
    {
        if (ui::BeginMenu("File"))
        {
            save = ui::MenuItem("Save Project");
            if (ui::MenuItem("Save Project As"))
            {
                save = true;
                projectFilePath_.Clear();
            }

            if (ui::MenuItem("Open Project"))
            {
                const char* patterns[] = {"*.xml"};
                projectFilePath_ = tinyfd_openFileDialog("Open Project", ".", 1, patterns, "XML Files", 0);
                LoadProject(projectFilePath_);
            }

            ui::Separator();

            if (ui::MenuItem("New Scene"))
                CreateNewScene();

            ui::Separator();

            if (ui::MenuItem("Exit"))
                engine_->Exit();

            ui::EndMenu();
        }

        if (!activeView_.Expired())
        {
            save |= ui::Button(ICON_FA_FLOPPY_O);
            ui::SameLine();
            if (ui::IsItemHovered())
                ui::SetTooltip("Save");
            ui::TextUnformatted("|");
            ui::SameLine();
            activeView_->RenderGizmoButtons();
            SendEvent(E_EDITORTOOLBARBUTTONS);
        }

        ui::EndMainMenuBar();
    }

    if (save)
    {
        if (projectFilePath_.Empty())
        {
            const char* patterns[] = {"*.xml"};
            projectFilePath_ = tinyfd_saveFileDialog("Save Project As", ".", 1, patterns, "XML Files");
        }
        SaveProject(projectFilePath_);
        for (auto& sceneView: sceneViews_)
            sceneView->SaveScene();
    }
}

SceneView* Editor::CreateNewScene(XMLElement project)
{
    SharedPtr<SceneView> sceneView;
    StringHash id;

    if (project.IsNull())
        id = idPool_.NewID();           // Make new ID only if scene is not being loaded from a project.

    if (sceneViews_.Empty())
        sceneView = new SceneView(context_, id, "Hierarchy", ui::Slot_Right);
    else
        sceneView = new SceneView(context_, id, sceneViews_.Back()->GetUniqueTitle(), ui::Slot_Tab);

    if (project.NotNull())
    {
        sceneView->LoadProject(project);
        if (!idPool_.TakeID(sceneView->GetID()))
        {
            URHO3D_LOGERRORF("Scene loading failed because unique id %s is already taken",
                sceneView->GetID().ToString().CString());
            return nullptr;
        }
    }

    // In order to render scene to a texture we must add a dummy node to scene rendered to a screen, which has material
    // pointing to scene texture. This object must also be visible to main camera.
    sceneViews_.Push(sceneView);
    return sceneView;
}

bool Editor::IsActive(Scene* scene)
{
    if (scene == nullptr || activeView_.Null())
        return false;

    return activeView_->GetScene() == scene && activeView_->IsActive();
}

}