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

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Application.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsDefs.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/SystemUI/Widgets/AttributeInspector.h>
#include <Urho3D/UI/UI.h>

#include <tinyfiledialogs/tinyfiledialogs.h>
#include <ImGui/imgui_internal.h>
#include <IconFontCppHeaders/IconsFontAwesome.h>

#include <unordered_map>

#include "UndoManager.h"


using namespace std::placeholders;

#define ENUM_FLAGS(t) \
inline t operator|(t a, t b) { return static_cast<t>(static_cast<size_t>(a) | static_cast<size_t>(b)); }\
inline t operator|=(t& a, t b) { a = static_cast<t>(static_cast<size_t>(a) | static_cast<size_t>(b)); return a; }

namespace Urho3D
{

enum ResizeType
{
    RESIZE_NONE = 0,
    RESIZE_LEFT = 1,
    RESIZE_RIGHT = 2,
    RESIZE_TOP = 4,
    RESIZE_BOTTOM = 8,
    RESIZE_MOVE = 16,
};

ENUM_FLAGS(ResizeType);

enum TransformSelectorFlags
{
    TSF_NONE = 0,
    TSF_NOHORIZONTAL = 1,
    TSF_NOVERTICAL = 2,
    TSF_HIDEHANDLES = 4,
};

ENUM_FLAGS(TransformSelectorFlags);

inline unsigned MakeHash(const ResizeType& value)
{
    return value;
}

class TransformSelector : public Object
{
URHO3D_OBJECT(TransformSelector, Object);
public:
    /// A flag indicationg type of resize action currently in progress
    ResizeType resizing_ = RESIZE_NONE;
    /// A cache of system cursors
    HashMap<ResizeType, SDL_Cursor*> cursors;
    /// Default cursor shape
    SDL_Cursor* cursorArrow;
    /// Flag indicating that this selector set cursor handle
    bool ownsCursor_ = false;

    explicit TransformSelector(Context* context)
        : Object(context)
    {
        cursors[RESIZE_MOVE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
        cursors[RESIZE_LEFT] = cursors[RESIZE_RIGHT] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
        cursors[RESIZE_BOTTOM] = cursors[RESIZE_TOP] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
        cursors[RESIZE_TOP | RESIZE_LEFT] = cursors[RESIZE_BOTTOM | RESIZE_RIGHT] = SDL_CreateSystemCursor(
            SDL_SYSTEM_CURSOR_SIZENWSE);
        cursors[RESIZE_TOP | RESIZE_RIGHT] = cursors[RESIZE_BOTTOM | RESIZE_LEFT] = SDL_CreateSystemCursor(
            SDL_SYSTEM_CURSOR_SIZENESW);
        cursorArrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    }

    bool RenderHandle(Vector2 screenPos, int wh, TransformSelectorFlags flags)
    {
        IntRect rect(
            static_cast<int>(screenPos.x_ - wh / 2),
            static_cast<int>(screenPos.y_ - wh / 2),
            static_cast<int>(screenPos.x_ + wh / 2),
            static_cast<int>(screenPos.y_ + wh / 2)
        );

        if (!(flags & TSF_HIDEHANDLES))
        {
            ui::GetWindowDrawList()->AddRectFilled({(float)rect.left_, (float)rect.top_},
                                                   {(float)rect.right_, (float)rect.bottom_},
                                                   ui::GetColorU32(ToImGui(Color::RED)));
        }

        auto input = context_->GetInput();
        return rect.IsInside(input->GetMousePosition()) == INSIDE;
    }

    bool OnUpdate(Rect screenRect, Rect& delta, TransformSelectorFlags flags = TSF_NONE)
    {
        auto input = context_->GetInput();
        bool wasNotMoving = resizing_ == RESIZE_NONE;
        bool can_resize_horizontal = !(flags & TSF_NOHORIZONTAL);
        bool can_resize_vertical = !(flags & TSF_NOVERTICAL);

        // Draw rect around selected element
        ui::GetWindowDrawList()->AddRect(
            ToImGui(screenRect.min_),
            ToImGui(screenRect.max_),
            ui::GetColorU32(ToImGui(Color::GREEN))
        );

        auto size = screenRect.max_ - screenRect.min_;
        auto handle_size = static_cast<int>(Max(Min(Min(size.x_ / 4, size.y_ / 4), 8), 2));
        ResizeType resizing = RESIZE_NONE;
        if (RenderHandle(screenRect.min_ + size / 2, handle_size, flags))
            resizing = RESIZE_MOVE;

        if (can_resize_horizontal && can_resize_vertical)
        {
            if (RenderHandle(screenRect.min_, handle_size, flags))
                resizing = RESIZE_LEFT | RESIZE_TOP;
            if (RenderHandle(screenRect.min_ + Vector2(0, size.y_), handle_size, flags))
                resizing = RESIZE_LEFT | RESIZE_BOTTOM;
            if (RenderHandle(screenRect.min_ + Vector2(size.x_, 0), handle_size, flags))
                resizing = RESIZE_TOP | RESIZE_RIGHT;
            if (RenderHandle(screenRect.max_, handle_size, flags))
                resizing = RESIZE_BOTTOM | RESIZE_RIGHT;
        }

        if (can_resize_horizontal)
        {
            if (RenderHandle(screenRect.min_ + Vector2(0, size.y_ / 2), handle_size, flags))
                resizing = RESIZE_LEFT;
            if (RenderHandle(screenRect.min_ + Vector2(size.x_, size.y_ / 2), handle_size, flags))
                resizing = RESIZE_RIGHT;
        }

        if (can_resize_vertical)
        {
            if (RenderHandle(screenRect.min_ + Vector2(size.x_ / 2, 0), handle_size, flags))
                resizing = RESIZE_TOP;
            if (RenderHandle(screenRect.min_ + Vector2(size.x_ / 2, size.y_), handle_size, flags))
                resizing = RESIZE_BOTTOM;
        }

        if (resizing == RESIZE_NONE)
        {
            if (ownsCursor_)
            {
                SDL_SetCursor(cursorArrow);
                ownsCursor_ = false;
            }
        }
        else
        {
            SDL_SetCursor(cursors[resizing]);
            ownsCursor_ = true;
        }

        if (input->GetMouseButtonDown(MOUSEB_LEFT))
        {
            // Start resizing only when resize is not in progress.
            if (wasNotMoving)
                resizing_ = resizing;
        }

        Vector2 d = Vector2(input->GetMouseMove().x_, input->GetMouseMove().y_);
        if (resizing_ != RESIZE_NONE)
        {
            bool mouseButtonReleased = !input->GetMouseButtonDown(MOUSEB_LEFT);
            if (wasNotMoving)
                SendEvent("ResizeStart");

            if (mouseButtonReleased)
            {
                resizing_ = RESIZE_NONE;
                SendEvent("ResizeEnd");
            }
            else
            {
                if (resizing_ & RESIZE_MOVE)
                {
                    delta.min_ += d;
                    delta.max_ += d;
                }
                else
                {
                    if (resizing_ & RESIZE_LEFT)
                        delta.min_ += Vector2(d.x_, 0);
                    else if (resizing_ & RESIZE_RIGHT)
                        delta.max_ += Vector2(d.x_, 0);

                    if (resizing_ & RESIZE_TOP)
                        delta.min_ += Vector2(0, d.y_);
                    else if (resizing_ & RESIZE_BOTTOM)
                        delta.max_ += Vector2(0, d.y_);
                }
                return true;
            }
        }
        return false;
    }

    bool is_active() const
    { return resizing_ != RESIZE_NONE; }
};

class UIEditor : public Application
{
URHO3D_OBJECT(UIEditor, Application);
public:
    SharedPtr<Scene> scene_;
    WeakPtr<UIElement> selectedElement_;
    WeakPtr<Camera> camera_;
    UndoManager undo_;
    String currentFilePath_;
    String currentStyleFilePath_;
    bool showInternal_ = false;
    ResizeType resizing_ = RESIZE_NONE;
    SharedPtr<XMLFile> styleFile_;
    Vector<String> styleNames_;
    bool hideResizeHandles_ = false;
    String textureSelectorAttribute_;
    SharedPtr<TransformSelector> uiElementTransform_;
    SharedPtr<TransformSelector> textureRectTransform_;
    ImVec2 textureWindowPos_;
    float textureWindowScale_ = 1.f;
    WeakPtr<UIElement> rootElement_;
    AttributeInspector inspector_;

    explicit UIEditor(Context* context) : Application(context), undo_(context), inspector_(context)
    {
    }

    void Setup() override
    {
        engineParameters_[EP_WINDOW_TITLE] = GetTypeName();
        engineParameters_[EP_HEADLESS] = false;
        engineParameters_[EP_RESOURCE_PREFIX_PATHS] =
            ";" + context_->GetFileSystem()->GetProgramDir() + ";" + "../share/Urho3D/Resources";
        engineParameters_[EP_FULL_SCREEN] = false;
        engineParameters_[EP_WINDOW_HEIGHT] = 1080;
        engineParameters_[EP_WINDOW_WIDTH] = 1920;
        engineParameters_[EP_LOG_LEVEL] = LOG_DEBUG;
        engineParameters_[EP_WINDOW_RESIZABLE] = true;
    }

    void Start() override
    {
        rootElement_ = GetUI()->GetRoot();
        GetSubsystem<SystemUI>()->AddFont("Fonts/fontawesome-webfont.ttf", 0, {ICON_MIN_FA, ICON_MAX_FA, 0}, true);

        GetInput()->SetMouseMode(MM_FREE);
        GetInput()->SetMouseVisible(true);

        // Background color
        scene_ = new Scene(context_);
        scene_->CreateComponent<Octree>();
        scene_->CreateComponent<Zone>()->SetFogColor(Color(0.2f, 0.2f, 0.2f));

        camera_ = scene_->CreateChild("Camera")->CreateComponent<Camera>();
        camera_->SetOrthographic(true);
        camera_->GetNode()->SetPosition({0, 10, 0});
        camera_->GetNode()->LookAt({0, 0, 0});
        GetSubsystem<Renderer>()->SetViewport(0, new Viewport(context_, scene_, camera_));
        uiElementTransform_ = new TransformSelector(context_);
        textureRectTransform_ = new TransformSelector(context_);

        // Events
        SubscribeToEvent(E_SYSTEMUIFRAME, std::bind(&UIEditor::RenderSystemUI, this));
        SubscribeToEvent(E_DROPFILE, std::bind(&UIEditor::OnFileDrop, this, _2));
        SubscribeToEvent(uiElementTransform_, "ResizeStart", std::bind(&UIEditor::UIElementResizeTrack, this));
        SubscribeToEvent(uiElementTransform_, "ResizeEnd", std::bind(&UIEditor::UIElementResizeTrack, this));
        SubscribeToEvent(E_ATTRIBUTEINSPECTVALUEMODIFIED, std::bind(&UIEditor::UIElementTrackAttributes, this, _2));
        SubscribeToEvent(E_ATTRIBUTEINSPECTORMENU, std::bind(&UIEditor::AttributeMenu, this, _2));
        SubscribeToEvent(E_ATTRIBUTEINSPECTOATTRIBUTE, std::bind(&UIEditor::AttributeCustomize, this, _2));

        // UI style
        GetSystemUI()->ApplyStyleDefault(true, 1.0f);
        ui::GetStyle().WindowRounding = 3;

        // Arguments
        for (const auto& arg: GetArguments())
            LoadFile(arg);
    }

    void UIElementResizeTrack()
    {
        if (auto selected = GetSelected())
            undo_.TrackState(selected, {{"Position", selected->GetPosition()},
                                        {"Size",     selected->GetSize()}});
    }

    void UIElementTrackAttributes(VariantMap& args)
    {
        using namespace AttributeInspectorValueModified;

        if (auto selected = GetSelected())
        {
            if (args[P_SERIALIZABLE].GetPtr() != selected)
                return;

            auto* info = static_cast<AttributeInfo*>(args[P_ATTRIBUTEINFO].GetVoidPtr());
            // Make sure old value is on undo stack. Most of the time this is optional, but not always.
            undo_.TrackState(selected, info->name_, args[P_OLDVALUE]);
            undo_.TrackState(selected, info->name_, args[P_NEWVALUE]);
        }
    }

    void AttributeMenu(VariantMap& args)
    {
        using namespace AttributeInspectorMenu;

        if (auto selected = GetSelected())
        {
            auto* item = dynamic_cast<Serializable*>(args[P_SERIALIZABLE].GetPtr());
            auto* info = static_cast<AttributeInfo*>(args[P_ATTRIBUTEINFO].GetVoidPtr());

            Variant value = item->GetAttribute(info->name_);
            XMLElement styleAttribute;
            XMLElement styleXml;
            Variant styleVariant;
            GetStyleData(*info, styleXml, styleAttribute, styleVariant);

            if (styleVariant != value)
            {
                if (!styleVariant.IsEmpty())
                {
                    if (ui::MenuItem("Reset to style"))
                    {
                        undo_.TrackState(item, info->name_, value);
                        item->SetAttribute(info->name_, styleVariant);
                        item->ApplyAttributes();
                        undo_.TrackState(item, info->name_, styleVariant);
                    }
                }

                if (styleXml.NotNull())
                {
                    if (ui::MenuItem("Save to style"))
                    {
                        if (styleAttribute.IsNull())
                        {
                            styleAttribute = styleXml.CreateChild("attribute");
                            styleAttribute.SetAttribute("name", info->name_);
                        }
                        styleAttribute.SetVariant(value);
                    }
                }
            }

            if (styleAttribute.NotNull())
            {
                if (ui::MenuItem("Remove from style"))
                    styleAttribute.GetParent().RemoveChild(styleAttribute);
            }

            if (info->type_ == VAR_INTRECT && dynamic_cast<BorderImage*>(selected) != nullptr)
            {
                if (ui::MenuItem("Select in UI Texture"))
                    textureSelectorAttribute_ = info->name_;
            }
        }
    }

    void AttributeCustomize(VariantMap& args)
    {
        using namespace AttributeInspectorAttribute;

        if (auto selected = GetSelected())
        {
            auto* item = dynamic_cast<Serializable*>(args[P_SERIALIZABLE].GetPtr());
            auto* info = static_cast<AttributeInfo*>(args[P_ATTRIBUTEINFO].GetVoidPtr());

            Variant value = item->GetAttribute(info->name_);
            XMLElement styleAttribute;
            XMLElement styleXml;
            Variant styleVariant;
            GetStyleData(*info, styleXml, styleAttribute, styleVariant);

            if (info->name_ == "Color")
            {
                if (styleVariant.IsEmpty())
                {
                    int a = 2;
                }
            }

            if (!styleVariant.IsEmpty())
            {
                if (styleVariant == value)
                {
                    args[P_COLOR] = Color::GRAY;
                    args[P_TOOLTIP] = "Value inherited from style.";
                }
                else
                {
                    args[P_COLOR] = Color::GREEN;
                    args[P_TOOLTIP] = "Style value was modified.";
                }
            }
        }
    }

    void RenderSystemUI()
    {
        if (ui::BeginMainMenuBar())
        {
            if (ui::BeginMenu("File"))
            {
                if (ui::MenuItem(ICON_FA_FILE_TEXT " New"))
                    rootElement_->RemoveAllChildren();

                const char* filters[] = {"*.xml"};
                if (ui::MenuItem(ICON_FA_FOLDER_OPEN " Open"))
                {
                    auto filename = tinyfd_openFileDialog("Open file", ".", 2, filters, "XML files", 0);
                    if (filename)
                        LoadFile(filename);
                }

                if (ui::MenuItem(ICON_FA_FLOPPY_O " Save UI As") && rootElement_->GetNumChildren() > 0)
                {
                    if (auto path = tinyfd_saveFileDialog("Save UI file", ".", 1, filters, "XML files"))
                        SaveFileUI(path);
                }

                if (ui::MenuItem(ICON_FA_FLOPPY_O " Save Style As") && styleFile_.NotNull())
                {
                    if (auto path = tinyfd_saveFileDialog("Save Style file", ".", 1, filters, "XML files"))
                        SaveFileStyle(path);
                }

                ui::EndMenu();
            }

            if (ui::Button(ICON_FA_FLOPPY_O))
            {
                if (!currentFilePath_.Empty())
                    SaveFileUI(currentFilePath_);
                if (!styleFile_.Null())
                    SaveFileStyle(currentStyleFilePath_);
            }

            if (ui::IsItemHovered())
                ui::SetTooltip("Save current UI and style files.");

            ui::SameLine();

            if (ui::Button(ICON_FA_UNDO))
                undo_.Undo();

            if (ui::IsItemHovered())
                ui::SetTooltip("Undo.");
            ui::SameLine();

            if (ui::Button(ICON_FA_REPEAT))
                undo_.Redo();

            if (ui::IsItemHovered())
                ui::SetTooltip("Redo.");

            ui::SameLine();

            ui::Checkbox("Show Internal", &showInternal_);
            ui::SameLine();

            ui::Checkbox("Hide Resize Handles", &hideResizeHandles_);
            ui::SameLine();

            ui::EndMainMenuBar();
        }

        const auto menuBarHeight = 20.f;
        const auto leftPanelWidth = 300.f;
        const auto leftPanelRight = 400.f;
        const auto panelFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                ImGuiWindowFlags_NoTitleBar;

        auto windowHeight = (float)GetGraphics()->GetHeight();
        auto windowWidth = (float)GetGraphics()->GetWidth();
        IntVector2 rootPos(5, static_cast<int>(5 + menuBarHeight));
        IntVector2 rootSize(0, static_cast<int>(windowHeight) - 20);

        ui::SetNextWindowPos({0.f, menuBarHeight}, ImGuiCond_Always);
        ui::SetNextWindowSize({leftPanelWidth, windowHeight - menuBarHeight});
        if (ui::Begin("ElementTree", nullptr, panelFlags))
        {
            rootPos.x_ += static_cast<int>(ui::GetWindowWidth());
            RenderUiTree(rootElement_);
        }
        ui::End();


        ui::SetNextWindowPos({windowWidth - leftPanelRight, menuBarHeight}, ImGuiCond_Always);
        ui::SetNextWindowSize({leftPanelRight, windowHeight - menuBarHeight});
        if (ui::Begin("AttributeList", nullptr, panelFlags))
        {
            rootSize.x_ = static_cast<int>(windowWidth - rootPos.x_ - ui::GetWindowWidth());
            if (auto selected = GetSelected())
            {
                ui::Columns(2);

                // Label
                ui::TextUnformatted("Style");
                ui::NextColumn();

                // Style name
                auto type_style = GetAppliedStyle();
                ui::TextUnformatted(type_style.CString());
                ui::NextColumn();

                inspector_.RenderAttributes(selected);
            }
        }
        ui::End();

        rootElement_->SetSize(rootSize);
        rootElement_->SetPosition(rootPos);

        // Background window
        // Used for rendering various lines on top of UrhoUI.
        const auto backgroundTextWindowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
                                               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs;
        ui::SetNextWindowSize(ToImGui(context_->GetGraphics()->GetSize()), ImGuiCond_Always);
        if (ui::Begin("Background Window", nullptr, ImVec2(0, 0), 0, backgroundTextWindowFlags))
        {
            if (auto selected = GetSelected())
            {
                // Render element selection rect, resize handles, and handle element transformations.
                Rect screenRect(
                    Vector2(selected->GetScreenPosition()),
                    Vector2(selected->GetScreenPosition() + selected->GetSize())
                );
                Rect delta = Rect::ZERO;

                auto flags = TSF_NONE;
                if (hideResizeHandles_)
                    flags |= TSF_HIDEHANDLES;
                if (selected->GetMinSize().x_ == selected->GetMaxSize().x_)
                    flags |= TSF_NOHORIZONTAL;
                if (selected->GetMinSize().y_ == selected->GetMaxSize().y_)
                    flags |= TSF_NOVERTICAL;
                if (uiElementTransform_->OnUpdate(screenRect, delta, flags))
                {
                    auto size = delta.max_ - delta.min_;
                    selected->SetPosition(selected->GetPosition() + IntVector2(static_cast<int>(delta.min_.x_),
                                                                               static_cast<int>(delta.min_.y_)));
                    selected->SetSize(selected->GetSize() + IntVector2(static_cast<int>(size.x_),
                                                                       static_cast<int>(size.y_)));
                }
            }
        }
        ui::End();
        // Background window end

        auto input = context_->GetInput();
        if (!uiElementTransform_->is_active() && input->GetMouseButtonPress(MOUSEB_LEFT) ||
            input->GetMouseButtonPress(MOUSEB_RIGHT))
        {
            auto pos = input->GetMousePosition();
            auto clicked = GetUI()->GetElementAt(pos, false);
            if (!clicked && rootElement_->GetCombinedScreenRect().IsInside(pos) == INSIDE)
                clicked = rootElement_;

            if (clicked)
                SelectItem(clicked);
        }

        if (auto selected = GetSelected())
        {
            if (input->GetKeyPress(KEY_DELETE))
            {
                undo_.TrackRemoval(selected);
                selectedElement_->Remove();
                SelectItem(nullptr);
            }
        }

        // These interactions include root element, therefore GetSelected() is not used here.
        if (selectedElement_)
        {
            if (ui::BeginPopupContextVoid("Element Context Menu", 2))
            {
                if (ui::BeginMenu("Add Child"))
                {
                    const char* ui_types[] = {"BorderImage", "Button", "CheckBox", "Cursor", "DropDownList", "LineEdit",
                                              "ListView", "Menu", "ProgressBar", "ScrollBar", "ScrollView", "Slider",
                                              "Sprite", "Text", "ToolTip", "UIElement", "View3D", "Window", nullptr
                    };
                    for (auto i = 0; ui_types[i] != nullptr; i++)
                    {
                        // TODO: element creation with custom styles more usable.
                        if (input->GetKeyDown(KEY_SHIFT))
                        {
                            if (ui::BeginMenu(ui_types[i]))
                            {
                                for (auto j = 0; j < styleNames_.Size(); j++)
                                {
                                    if (ui::MenuItem(styleNames_[j].CString()))
                                    {
                                        SelectItem(selectedElement_->CreateChild(ui_types[i]));
                                        selectedElement_->SetStyle(styleNames_[j]);
                                        undo_.TrackCreation(selectedElement_);
                                    }
                                }
                                ui::EndMenu();
                            }
                        }
                        else
                        {
                            if (ui::MenuItem(ui_types[i]))
                            {
                                SelectItem(selectedElement_->CreateChild(ui_types[i]));
                                selectedElement_->SetStyleAuto();
                                undo_.TrackCreation(selectedElement_);
                            }
                        }
                    }
                    ui::EndMenu();
                }

                if (selectedElement_ != rootElement_)
                {
                    if (ui::MenuItem("Delete Element"))
                    {
                        undo_.TrackRemoval(selectedElement_);
                        selectedElement_->Remove();
                        SelectItem(nullptr);
                    }

                    if (ui::MenuItem("Bring To Front"))
                        selectedElement_->BringToFront();
                }
                ui::EndPopup();
            }

            if (!textureSelectorAttribute_.Empty())
            {
                auto selected = DynamicCast<BorderImage>(selectedElement_);
                bool open = selected.NotNull();
                if (open)
                {
                    auto tex = selected->GetTexture();
                    auto padding = ImGui::GetStyle().WindowPadding;
                    if (ui::Begin("Select Rect", &open,
                                  ImVec2(tex->GetWidth() + padding.x * 2, tex->GetHeight() + padding.y * 2)))
                    {
                        if (ui::IsWindowHovered())
                        {
                            float d = input->GetMouseMoveWheel();
                            if (d != 0.f)
                                textureWindowScale_ += 1.f / d;
                        }

                        ui::Image(tex, ImVec2(tex->GetWidth() * textureWindowScale_,
                                              tex->GetHeight() * textureWindowScale_));
                        textureWindowPos_ = ui::GetWindowPos();

                        if (!textureSelectorAttribute_.Empty())
                        {
                            IntRect rect = selectedElement_->GetAttribute(textureSelectorAttribute_).GetIntRect();
                            TransformSelectorFlags flags = TSF_NONE;
                            if (hideResizeHandles_)
                                flags |= TSF_HIDEHANDLES;

                            auto padding2 = ui::GetStyle().WindowPadding;
                            Rect screenRect, delta;
                            screenRect.min_ = Vector2(rect.left_, rect.top_) + Vector2(&padding2.x) +
                                              Vector2(0, ui::GetCurrentWindow()->TitleBarHeight()) +
                                              Vector2(textureWindowPos_.x, textureWindowPos_.y);
                            screenRect.max_ =
                                screenRect.min_ + Vector2(rect.right_ - rect.left_, rect.bottom_ - rect.top_);

                            if (textureRectTransform_->OnUpdate(screenRect, delta, flags))
                            {

                            }
                        }
                    }
                    ui::End();
                }

                if (!open)
                    textureSelectorAttribute_.Clear();
            }
        }

        if (!ui::IsAnyItemActive())
        {
            if (input->GetKeyDown(KEY_CTRL))
            {
                if (input->GetKeyPress(KEY_Y) || (input->GetKeyDown(KEY_SHIFT) && input->GetKeyPress(KEY_Z)))
                    undo_.Redo();
                else if (input->GetKeyPress(KEY_Z))
                    undo_.Undo();
            }
        }
    }

    void OnFileDrop(VariantMap& args)
    {
        LoadFile(args[DropFile::P_FILENAME].GetString());
    }

    String GetResourcePath(String filePath)
    {
        const static Vector<String> dataDirectories = {
            "Materials", "RenderPaths", "Shaders", "Techniques", "Textures", "Fonts", "Models", "Particle", "Scenes",
            "Textures", "Music", "Objects", "PostProcess", "Sounds", "UI"
        };

        for (;filePath.Length();)
        {
            filePath = GetParentPath(filePath);
            for (const auto& dataDirectory: dataDirectories)
            {
                if (GetFileSystem()->DirExists(filePath + dataDirectory))
                    return filePath;
            }
        }

        return "";
    }

    bool LoadFile(const String& filePath)
    {
        auto cache = GetSubsystem<ResourceCache>();
        String resourceDir;
        if (IsAbsolutePath(filePath))
        {
            if (!currentFilePath_.Empty())
            {
                resourceDir = GetResourcePath(currentFilePath_);
                if (!resourceDir.Empty())
                    cache->RemoveResourceDir(resourceDir);
            }

            resourceDir = GetResourcePath(filePath);
            if (!resourceDir.Empty())
            {
                if (!cache->GetResourceDirs().Contains(resourceDir))
                    cache->AddResourceDir(resourceDir);
            }
        }

        if (filePath.EndsWith(".xml", false))
        {
            SharedPtr<XMLFile> xml(new XMLFile(context_));
            bool loaded = false;
            if (IsAbsolutePath(filePath))
                loaded = xml->LoadFile(filePath);
            else
            {
                auto cacheFile = cache->GetFile(filePath);
                loaded = xml->Load(*cacheFile);
            }

            if (loaded)
            {
                if (xml->GetRoot().GetName() == "elements")
                {
                    // This is a style.
                    rootElement_->SetDefaultStyle(xml);
                    styleFile_ = xml;
                    currentStyleFilePath_ = filePath;

                    auto styles = styleFile_->GetRoot().SelectPrepared(XPathQuery("/elements/element"));
                    for (auto i = 0; i < styles.Size(); i++)
                    {
                        auto type = styles[i].GetAttribute("type");
                        if (type.Length() && !styleNames_.Contains(type))
                            styleNames_.Push(type);
                    }
                    Sort(styleNames_.Begin(), styleNames_.End());
                    UpdateWindowTitle();
                    return true;
                }
                else if (xml->GetRoot().GetName() == "element")
                {
                    // If element has style file specified - load it
                    auto styleFile = xml->GetRoot().GetAttribute("styleFile");
                    if (!styleFile.Empty())
                    {
                        if (!currentStyleFilePath_.Empty())
                        {
                            auto styleResourceDir = GetResourcePath(currentStyleFilePath_);
                            if (!styleResourceDir.Empty())
                                cache->RemoveResourceDir(styleResourceDir);
                        }
                        LoadFile(styleFile);
                    }

                    Vector<SharedPtr<UIElement>> children = rootElement_->GetChildren();
                    auto child = rootElement_->CreateChild(xml->GetRoot().GetAttribute("type"));
                    if (child->LoadXML(xml->GetRoot()))
                    {
                        child->SetStyleAuto();
                        currentFilePath_ = filePath;
                        UpdateWindowTitle();

                        for (const auto& oldChild : children)
                            oldChild->Remove();

                        return true;
                    }
                    else
                        child->Remove();
                }
            }
        }

        cache->RemoveResourceDir(resourceDir);
        tinyfd_messageBox("Error", "Opening XML file failed", "ok", "error", 1);
        return false;
    }

    bool SaveFileUI(const String& file_path)
    {
        if (file_path.EndsWith(".xml", false))
        {
            XMLFile xml(context_);
            XMLElement root = xml.CreateRoot("element");
            if (rootElement_->GetChild(0)->SaveXML(root))
            {
                // Remove internal UI elements
                auto result = root.SelectPrepared(XPathQuery("//element[@internal=\"true\"]"));
                for (auto el = result.FirstResult(); el.NotNull(); el = el.NextResult())
                    el.GetParent().RemoveChild(el);

                // Remove style="none"
                root.SelectPrepared(XPathQuery("//element[@style=\"none\"]"));
                for (auto el = result.FirstResult(); el.NotNull(); el = el.NextResult())
                    el.RemoveAttribute("style");

                File saveFile(context_, file_path, FILE_WRITE);
                xml.Save(saveFile);

                currentFilePath_ = file_path;
                UpdateWindowTitle();
                return true;
            }
        }

        tinyfd_messageBox("Error", "Saving UI file failed", "ok", "error", 1);
        return false;
    }

    bool SaveFileStyle(const String& file_path)
    {
        if (file_path.EndsWith(".xml", false) && styleFile_.NotNull())
        {
            File saveFile(context_, file_path, FILE_WRITE);
            styleFile_->Save(saveFile);

            currentStyleFilePath_ = file_path;
            UpdateWindowTitle();
            return true;
        }

        tinyfd_messageBox("Error", "Saving UI file failed", "ok", "error", 1);
        return false;
    }

    void RenderUiTree(UIElement* element)
    {
        auto& name = element->GetName();
        auto& type = element->GetTypeName();
        auto tooltip = "Type: " + type;
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        bool is_internal = element->IsInternal();
        if (is_internal && !showInternal_)
            return;
        else
            flags |= ImGuiTreeNodeFlags_DefaultOpen;

        if (showInternal_)
            tooltip += String("\nInternal: ") + (is_internal ? "true" : "false");

        if (element == selectedElement_)
            flags |= ImGuiTreeNodeFlags_Selected;

        if (ui::TreeNodeEx(element, flags, "%s", name.Length() ? name.CString() : type.CString()))
        {
            if (ui::IsItemHovered())
                ui::SetTooltip(tooltip.CString());

            if (ui::IsItemHovered() && ui::IsMouseClicked(0))
                SelectItem(element);

            for (const auto& child: element->GetChildren())
                RenderUiTree(child);
            ui::TreePop();
        }
    }

    String GetAppliedStyle(UIElement* element = nullptr)
    {
        if (element == nullptr)
            element = selectedElement_;

        if (element == nullptr)
            return "";

        auto applied_style = selectedElement_->GetAppliedStyle();
        if (applied_style.Empty())
            applied_style = selectedElement_->GetTypeName();
        return applied_style;
    }

    String GetBaseName(const String& full_path)
    {
        auto parts = full_path.Split('/');
        return parts.At(parts.Size() - 1);
    }

    void UpdateWindowTitle()
    {
        String window_name = "UrhoUIEditor";
        if (!currentFilePath_.Empty())
            window_name += " - " + GetBaseName(currentFilePath_);
        if (!currentStyleFilePath_.Empty())
            window_name += " - " + GetBaseName(currentStyleFilePath_);
        context_->GetGraphics()->SetWindowTitle(window_name);
    }

    void SelectItem(UIElement* current)
    {
        if (resizing_)
            return;

        if (current == nullptr)
            textureSelectorAttribute_.Clear();

        selectedElement_ = current;
    }

    UIElement* GetSelected() const
    {
        // Can not select root widget
        if (selectedElement_ == GetUI()->GetRoot())
            return nullptr;

        return selectedElement_;
    }

    void GetStyleData(const AttributeInfo& info, XMLElement& style, XMLElement& attribute, Variant& value)
    {
        auto styleFile = selectedElement_->GetDefaultStyle();
        if (styleFile == nullptr)
            return;

        static XPathQuery xpAttribute("attribute[@name=$name]", "name:String");
        static XPathQuery xpStyle("/elements/element[@type=$type]", "type:String");

        value = Variant();
        xpAttribute.SetVariable("name", info.name_);

        auto styleName = GetAppliedStyle();

        do
        {
            // Get current style
            xpStyle.SetVariable("type", styleName);
            style = styleFile->GetRoot().SelectSinglePrepared(xpStyle);
            // Look for attribute in current style
            attribute = style.SelectSinglePrepared(xpAttribute);
            // Go up in style hierarchy
            styleName = style.GetAttribute("Style");
        } while (attribute.IsNull() && !styleName.Empty() && !style.IsNull());


        if (!attribute.IsNull())
        {
            value = attribute.GetVariantValue(info.enumNames_ ? VAR_STRING : info.type_);
            if (info.enumNames_)
            {
                for (auto i = 0; info.enumNames_[i]; i++)
                {
                    if (value.GetString() == info.enumNames_[i])
                    {
                        value = i;
                        break;
                    }
                }
            }
        }
    }
};

}

URHO3D_DEFINE_APPLICATION_MAIN(Urho3D::UIEditor);
