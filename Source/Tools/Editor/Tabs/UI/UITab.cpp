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

#include <EASTL/sort.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/UI/Window.h>
#include <Urho3D/UI/UI.h>
#include <Toolbox/IO/ContentUtilities.h>
#include <Toolbox/SystemUI/Widgets.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include "EditorEvents.h"
#include "Editor.h"
#include "Widgets.h"
#include "UITab.h"
#include "Tabs/HierarchyTab.h"
#include "Tabs/InspectorTab.h"

namespace Urho3D
{

UITab::UITab(Context* context)
    : BaseResourceTab(context)
{
    SetID(GenerateUUID());
    SetTitle("New UI Layout");
    windowFlags_ = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    noContentPadding_ = true;

    texture_ = new Texture2D(context_);
    texture_->SetFilterMode(FILTER_BILINEAR);
    texture_->SetAddressMode(COORD_U, ADDRESS_CLAMP);
    texture_->SetAddressMode(COORD_V, ADDRESS_CLAMP);
    texture_->SetNumLevels(1);

    rootElement_ = new RootUIElement(context_);
    rootElement_->SetTraversalMode(TM_BREADTH_FIRST);
    rootElement_->SetEnabled(true);
    rootElement_->SetElementEventSender(true);

    offScreenUI_ = new UI(context_);
    offScreenUI_->SetRoot(rootElement_);
    offScreenUI_->SetRenderTarget(texture_, Color::BLACK);

    undo_.Connect(static_cast<UIElement*>(rootElement_.Get()));

    SubscribeToEvent(E_ATTRIBUTEINSPECTORMENU, std::bind(&UITab::AttributeMenu, this, _2));
    SubscribeToEvent(E_ATTRIBUTEINSPECTOATTRIBUTE, std::bind(&UITab::AttributeCustomize, this, _2));

    AutoLoadDefaultStyle();
}

void UITab::RenderHierarchy()
{
    if (rootElement_.Null())
        return;

    ui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 10);
    RenderNodeTree(rootElement_);
    ui::PopStyleVar();
}

void UITab::RenderNodeTree(UIElement* element)
{
    SharedPtr<UIElement> elementRef(element);
    ea::string name = element->GetName();
    ea::string type = element->GetTypeName();
    ea::string tooltip = "Type: " + type;
    if (name.empty())
        name = type;
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    bool isInternal = element->IsInternal();
    if (isInternal && !showInternal_)
        return;
    else
        flags |= ImGuiTreeNodeFlags_DefaultOpen;

    if (showInternal_)
        tooltip += ea::string("\nInternal: ") + (isInternal ? "true" : "false");

    if (element == selectedElement_)
        flags |= ImGuiTreeNodeFlags_Selected;

    ui::Image(element->GetTypeName());
    ui::SameLine();

    auto treeExpanded = ui::TreeNodeEx(element, flags, "%s", name.c_str());

    if (ui::BeginDragDropSource())
    {
        ui::SetDragDropVariant("ptr", (void*)element);
        ui::Text("%s", name.c_str());
        ui::EndDragDropSource();
    }

    if (ui::BeginDragDropTarget())
    {
        // Reparent by drag&drop, insert as first item
        const Variant& payload = ui::AcceptDragDropVariant("ptr");
        if (!payload.IsEmpty())
        {
            SharedPtr<UIElement> child((UIElement*)payload.GetVoidPtr());
            if (child && child != element)
            {
                child->Remove();    // Needed for reordering under the same parent.
                element->InsertChild(0, child);
            }
        }
        ui::EndDragDropTarget();
    }

    if (treeExpanded)
    {
        if (ui::IsItemHovered())
            ui::SetTooltip("%s", tooltip.c_str());

        if (ui::IsItemHovered())
        {
            if (ui::IsMouseClicked(MOUSEB_LEFT) || ui::IsMouseClicked(MOUSEB_RIGHT))
            {
                SelectItem(element);
                if (ui::IsMouseClicked(MOUSEB_RIGHT))
                    ui::OpenPopup("Element Context Menu");
            }
        }

        RenderElementContextMenu();

        // Context menu may delete this element
        bool wasDeleted = (flags & ImGuiTreeNodeFlags_Selected) && !selectedElement_;
        if (!wasDeleted)
        {
            // Do not use element->GetChildren() because child may be deleted during this loop.
            ea::vector<UIElement*> children;
            element->GetChildren(children);
            for (const auto& child: children)
                RenderNodeTree(child);
        }

        ui::TreePop();
    }

    ImRect bb{ui::GetItemRectMin(), ui::GetItemRectMax()};
    bb.Min.y = bb.Max.y;
    bb.Max.y += 2;
    if (ui::BeginDragDropTargetCustom(bb, ui::GetID("reorder")))
    {
        // Reparent by drag&drop between elements, insert after current item
        const Variant& payload = ui::AcceptDragDropVariant("ptr");
        if (!payload.IsEmpty())
        {
            SharedPtr<UIElement> child((UIElement*)payload.GetVoidPtr());
            if (child && child != element)
            {
                child->Remove();    // Needed for reordering under the same parent.
                auto index = element->GetParent()->FindChild(element) + 1;
                element->GetParent()->InsertChild(index, child);
            }
        }
        ui::EndDragDropTarget();
    }
}

bool UITab::RenderWindowContent()
{
    RenderToolbarButtons();

    ImGuiWindow* window = ui::GetCurrentWindow();
    ImGuiViewport* viewport = window->Viewport;
    ImRect rect = ImRound(window->ContentRegionRect);
    // Correct content rect to not overlap buttons.
    rect.Min.y += ui::GetCursorPosY();
    IntVector2 textureSize{
        static_cast<int>(IM_ROUND(rect.GetWidth() * viewport->DpiScale)),
        static_cast<int>(IM_ROUND(rect.GetHeight() * viewport->DpiScale))
    };
    if (textureSize.x_ != texture_->GetWidth() || textureSize.y_ != texture_->GetHeight())
    {
        ImVec2 offset = (rect.Min - viewport->Pos) * viewport->DpiScale;
        rootElement_->SetOffset({static_cast<int>(IM_ROUND(offset.x)), static_cast<int>(IM_ROUND(offset.y))});
        offScreenUI_->SetCustomSize(textureSize);
    }

    ui::SetCursorScreenPos(rect.Min);
    ImVec2 contentSize = rect.GetSize();
    ui::BeginChild("UI view", contentSize, false, windowFlags_);
    ui::Image(texture_, contentSize);

    if (auto selected = GetSelected())
    {
        // Render element selection rect, resize handles, and handle element transformations.
        ImVec2 elementPos(Vector2(selected->GetScreenPosition()) / viewport->DpiScale);
        ImVec2 elementSize(Vector2(selected->GetSize()) / viewport->DpiScale);
        ImRect screenRect{elementPos + rect.Min, elementPos + rect.Min + elementSize};
        ImRect delta;

        ui::TransformSelectorFlags flags = ui::TSF_NONE;
        if (hideResizeHandles_)
            flags |= ui::TSF_HIDEHANDLES;
        if (selected->GetMinSize().x_ == selected->GetMaxSize().x_)
            flags |= ui::TSF_NOHORIZONTAL;
        if (selected->GetMinSize().y_ == selected->GetMaxSize().y_)
            flags |= ui::TSF_NOVERTICAL;

        struct State
        {
            bool resizeActive_ = false;
            IntVector2 resizeStartPos_;
            IntVector2 resizeStartSize_;
        };
        auto* s = ui::GetUIState<State>();

        if (ui::TransformRect(screenRect, delta, flags))
        {
            if (!s->resizeActive_)
            {
                s->resizeActive_ = true;
                s->resizeStartPos_ = selected->GetPosition();
                s->resizeStartSize_ = selected->GetSize();
            }
            IntRect pixelDelta{
                static_cast<int>(IM_ROUND(delta.Min.x * viewport->DpiScale)),
                static_cast<int>(IM_ROUND(delta.Min.y * viewport->DpiScale)),
                static_cast<int>(IM_ROUND(delta.GetWidth() * viewport->DpiScale)),
                static_cast<int>(IM_ROUND(delta.GetHeight() * viewport->DpiScale)),
            };
            selected->SetPosition(selected->GetPosition() + pixelDelta.Min());
            selected->SetSize(selected->GetSize() + pixelDelta.Max());
        }

        if (s->resizeActive_ && !ui::IsItemActive())
        {
            s->resizeActive_ = false;
            undo_.Add<UndoEditAttribute>(selected, "Position", s->resizeStartPos_, selected->GetPosition());
            undo_.Add<UndoEditAttribute>(selected, "Size", s->resizeStartSize_, selected->GetSize());
        }
    }

    RenderRectSelector();
    ui::EndChild();

    BaseClassName::RenderWindowContent();

    return true;
}

void UITab::RenderToolbarButtons()
{
    ui::SetCursorPos(ui::GetCursorPos() + ImVec2{4, 4});

    if (ui::EditorToolbarButton(ICON_FA_SAVE, "Save"))
        SaveResource();

    ui::SameLine(0, 3.f);

    ui::Checkbox("Show Internal", &showInternal_);
    ui::SameLine();
    ui::Checkbox("Hide Resize Handles", &hideResizeHandles_);

    ui::SameLine(0, 3.f);
    ui::SetCursorPosY(ui::GetCursorPosY() + 4);
}

void UITab::OnActiveUpdate()
{
    if (!ui::IsAnyItemActive())
    {
        if (auto selected = GetSelected())
        {
            if (ui::IsKeyPressed(KEY_DELETE))
            {
                selected->Remove();
                SelectItem(nullptr);    // Undo system still holds a reference to removed element therefore we
                                                // must manually clear selectedElement_.
            }
        }
    }

    if (!ui::IsAnyItemActive() && !ui::IsAnyItemHovered())
    {
        if (ui::IsMouseReleased(MOUSEB_LEFT) || ui::IsMouseReleased(MOUSEB_RIGHT))
        {
            Input* input = GetSubsystem<Input>();
            IntVector2 pos = rootElement_->ScreenToElement(input->GetMousePosition());
            UIElement* clicked = offScreenUI_->GetElementAt(pos, false);
            if (!clicked && rootElement_->GetCombinedScreenRect().IsInside(pos) == INSIDE && !ui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
                clicked = rootElement_;

            if (clicked)
            {
                SelectItem(clicked);

                if (ui::IsMouseReleased(MOUSEB_RIGHT))
                    ui::OpenPopup("Element Context Menu");
            }
        }
    }

    RenderElementContextMenu();
}

bool UITab::LoadResource(const ea::string& resourcePath)
{
    if (!BaseClassName::LoadResource(resourcePath))
        return false;

    if (GetContentType(context_, resourcePath) != CTYPE_UILAYOUT)
    {
        URHO3D_LOGERRORF("%s is not a UI layout.", resourcePath.c_str());
        return false;
    }

    UndoTrackGuard noTrack(undo_, false);

    auto cache = GetSubsystem<ResourceCache>();
    rootElement_->RemoveAllChildren();

    UIElement* layoutElement = nullptr;
    if (resourcePath.ends_with(".xml"))
    {
        SharedPtr<XMLFile> file(cache->GetResource<XMLFile>(resourcePath));
        if (file)
        {
            ea::string type = file->GetRoot().GetAttribute("type");
            if (type.empty())
                type = "UIElement";
            auto* child = rootElement_->CreateChild(StringHash(type));
            if (child->LoadXML(file->GetRoot()))
                layoutElement = child;
            else
                child->Remove();
        }
        else
        {
            URHO3D_LOGERRORF("Loading file %s failed.", resourcePath.c_str());
            cache->ReleaseResource(XMLFile::GetTypeStatic(), resourcePath, true);
            return false;
        }
    }
    else
    {
        URHO3D_LOGERROR("Unsupported format.");
        cache->ReleaseResource(XMLFile::GetTypeStatic(), resourcePath, true);
        return false;
    }

    if (layoutElement != nullptr)
    {
        layoutElement->SetStyleAuto();

        // Must be disabled because it interferes with ui element resizing
        if (auto* window = layoutElement->Cast<Window>())
        {
            window->SetMovable(false);
            window->SetResizable(false);
        }
    }
    else
    {
        URHO3D_LOGERRORF("Loading UI layout %s failed.", resourcePath.c_str());
        cache->ReleaseResource(XMLFile::GetTypeStatic(), resourcePath, true);
        return false;
    }

    undo_.Clear();
    lastUndoIndex_ = undo_.Index();

    return true;
}

bool UITab::SaveResource()
{
    if (!BaseClassName::SaveResource())
        return false;

    if (rootElement_->GetNumChildren() < 1)
        return false;

    auto styleFile = rootElement_->GetDefaultStyle();
    if (styleFile == nullptr)
        return false;

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    ea::string savePath = cache->GetResourceFileName(resourceName_);
    cache->ReleaseResource(XMLFile::GetTypeStatic(), resourceName_);

    if (resourceName_.ends_with(".xml"))
    {
        XMLFile xml(context_);
        XMLElement root = xml.CreateRoot("element");
        if (rootElement_->GetChild(0)->SaveXML(root))
        {
            // Remove internal UI elements
            auto result = root.SelectPrepared(XPathQuery("//element[@internal=\"true\"]"));
            for (auto el = result.FirstResult(); el.NotNull(); el = el.NextResult())
            {
                // Remove only top level internal elements.
                bool internalParent = false;
                auto parent = el.GetParent();
                do
                {
                    internalParent = parent.HasAttribute("internal") && parent.GetAttribute("internal") == "true";
                    parent = parent.GetParent();
                } while (!internalParent && parent.NotNull());

                if (!internalParent)
                    el.Remove();
            }

            // Remove style="none"
            result = root.SelectPrepared(XPathQuery("//element[@style=\"none\"]"));
            for (auto el = result.FirstResult(); el.NotNull(); el = el.NextResult())
                el.RemoveAttribute("style");

            // TODO: remove attributes with values matching style
            // TODO: remove attributes with default values

            File saveFile(context_, savePath, FILE_WRITE);
            if (!xml.Save(saveFile))
                return false;
        }
        else
            return false;
    }
    else
    {
        URHO3D_LOGERROR("Unsupported format.");
        return false;
    }

    // Save style
    savePath = cache->GetResourceFileName(styleFile->GetName());
    File saveFile(context_, savePath, FILE_WRITE);
    if (!styleFile->Save(saveFile))
        return false;

    SendEvent(E_EDITORRESOURCESAVED);

    return true;
}

UIElement* UITab::GetSelected() const
{
    // Can not select root widget
    if (selectedElement_ == rootElement_)
        return nullptr;

    return selectedElement_;
}

void UITab::SelectItem(UIElement* current)
{
    if (current == nullptr)
        textureSelectorAttribute_.clear();

    selectedElement_ = current;

    auto* editor = GetSubsystem<Editor>();
    editor->ClearInspector();
    editor->Inspect(current);
    editor->GetTab<HierarchyTab>()->SetProvider(this);
}

void UITab::AutoLoadDefaultStyle()
{
    styleNames_.clear();
    auto cache = GetSubsystem<ResourceCache>();
    auto fs = GetSubsystem<FileSystem>();
    for (const auto& dir: cache->GetResourceDirs())
    {
        ea::vector<ea::string> items;
        fs->ScanDir(items, dir + "UI", "", SCAN_FILES, false);

        for (const auto& fileName : items)
        {
            auto resourcePath = dir + "UI/" + fileName;
            // Icons file is also a style file. Without this ugly workaround sometimes wrong style gets applied.
            if (GetContentType(context_, resourcePath) == CTYPE_UISTYLE && !resourcePath.ends_with("Icons.xml"))
            {
                auto* style = cache->GetResource<XMLFile>(resourcePath);
                rootElement_->SetDefaultStyle(style);

                auto styles = style->GetRoot().SelectPrepared(XPathQuery("/elements/element"));
                for (auto i = 0; i < styles.Size(); i++)
                {
                    auto type = styles[i].GetAttribute("type");
                    if (type.length() && !styleNames_.contains(type) &&
                        styles[i].GetAttribute("auto").to_lower() == "false")
                        styleNames_.push_back(type);
                }
                break;
            }
        }
    }
    ea::quick_sort(styleNames_.begin(), styleNames_.end());
}

void UITab::RenderElementContextMenu()
{
    if (ui::BeginPopup("Element Context Menu"))
    {
        if (ui::BeginMenu("Create Child"))
        {
            auto components = GetSubsystem<Editor>()->GetObjectsByCategory("UI");
            ea::quick_sort(components.begin(), components.end());

            for (const ea::string& component : components)
            {
                // TODO: element creation with custom styles more usable.
                if (ui::IsKeyDown(KEY_SHIFT))
                {
                    ui::Image(component);
                    ui::SameLine();
                    if (ui::BeginMenu(component.c_str()))
                    {
                        for (auto j = 0; j < styleNames_.size(); j++)
                        {
                            if (ui::MenuItem(styleNames_[j].c_str()))
                            {
                                SelectItem(selectedElement_->CreateChild(StringHash(component)));
                                selectedElement_->SetStyle(styleNames_[j]);
                            }
                        }
                        ui::EndMenu();
                    }
                }
                else
                {
                    ui::Image(component);
                    ui::SameLine();
                    if (ui::MenuItem(component.c_str()))
                    {
                        SelectItem(selectedElement_->CreateChild(StringHash(component)));
                        selectedElement_->SetStyleAuto();
                    }
                }
            }
            ui::EndMenu();
        }

        if (auto selected = GetSelected())
        {
            if (ui::MenuItem("Delete Element"))
            {
                selected->Remove();
                SelectItem(nullptr);
            }

            if (ui::MenuItem("Bring To Front"))
                selected->BringToFront();
        }
        ui::EndPopup();
    }
}

ea::string UITab::GetAppliedStyle(UIElement* element)
{
    if (element == nullptr)
        element = selectedElement_;

    if (element == nullptr)
        return "";

    auto appliedStyle = selectedElement_->GetAppliedStyle();
    if (appliedStyle.empty())
        appliedStyle = selectedElement_->GetTypeName();
    return appliedStyle;
}

void UITab::RenderRectSelector()
{
    const ImGuiStyle& style = ui::GetStyle();
    auto* selected = GetSelected() ? GetSelected()->Cast<BorderImage>() : nullptr;

    if (textureSelectorAttribute_.empty() || selected == nullptr)
        return;

    struct State
    {
        bool isResizing_ = false;
        IntRect startRect_;
        ImRect rect_;
        int textureScale_ = 1;
        int windowFlags_ = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
    };
    auto* s = ui::GetUIState<State>();

    bool open = true;
    auto* texture = static_cast<Texture2D*>(selected->GetTexture());
    texture->SetFilterMode(FILTER_NEAREST);    // Texture is better visible this way when zoomed in.
    if (ui::Begin("Select Rect", &open, s->windowFlags_))
    {
        ImGuiWindow* window = ui::GetCurrentWindow();
        ImGuiViewport* viewport = window->Viewport;

        ui::SliderInt("Zoom", &s->textureScale_, 1, 5);
        ImVec2 imageSize{
            (float)texture->GetWidth() * (float)s->textureScale_ / viewport->DpiScale,
            (float)texture->GetHeight() * (float)s->textureScale_ / viewport->DpiScale,
        };
        ui::Image(texture, imageSize);
        ImVec2 imagePos = ui::GetItemRectMin();

        // Disable dragging of window if mouse is hovering texture.
        if (ui::IsItemHovered())
            s->windowFlags_ |= ImGuiWindowFlags_NoMove;
        else
            s->windowFlags_ &= ~ImGuiWindowFlags_NoMove;

        IntRect rect = selectedElement_->GetAttribute(textureSelectorAttribute_).GetIntRect();
        ImRect selectorRect;
        if (s->isResizing_)
            selectorRect = s->rect_;
        else
        {
            selectorRect = {
                imagePos + ImVec2(Vector2(rect.Min()) * (float)s->textureScale_ / viewport->DpiScale),
                imagePos + ImVec2(Vector2(rect.Max()) * (float)s->textureScale_ / viewport->DpiScale)
            };
        }

        ui::TransformSelectorFlags flags = ui::TSF_NONE;
        if (hideResizeHandles_)
            flags |= ui::TSF_HIDEHANDLES;

        ImRect delta;
        if (ui::TransformRect(selectorRect, delta, flags))
        {
            if (!s->isResizing_)
            {
                s->isResizing_ = true;
                s->startRect_ = rect;
                s->rect_ = selectorRect;
            }

            if (ui::IsItemActive())
            {
                ImVec2 min = (selectorRect.Min - imagePos) / (float)s->textureScale_ * viewport->DpiScale;
                ImVec2 max = (selectorRect.Max - imagePos) / (float)s->textureScale_ * viewport->DpiScale;
                IntRect currentRect{
                    IntVector2(Round(min.x), Round(min.y)),
                    IntVector2(Round(max.x), Round(max.y))
                };
                selectedElement_->SetAttribute(textureSelectorAttribute_, currentRect);
                s->rect_ = selectorRect;
            }
        }
        else if (!ui::IsItemActive() && s->isResizing_)
        {
            s->isResizing_ = false;
            undo_.Add<UndoEditAttribute>(selected, textureSelectorAttribute_, s->startRect_,
                                         selected->GetAttribute(textureSelectorAttribute_));
        }
    }
    ui::End();

    if (!open)
        textureSelectorAttribute_.clear();
}

Variant UITab::GetVariantFromXML(const XMLElement& attribute, const AttributeInfo& info) const
{
    Variant value = attribute.GetVariantValue(info.enumNames_ ? VAR_STRING : info.type_);
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
    return value;
}

void UITab::GetStyleData(const AttributeInfo& info, XMLElement& style, XMLElement& attribute, Variant& value)
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
    } while (attribute.IsNull() && !styleName.empty() && !style.IsNull());


    if (!attribute.IsNull() && attribute.GetAttribute("type") != "None")
        value = GetVariantFromXML(attribute, info);
}

void UITab::AttributeMenu(VariantMap& args)
{
    using namespace AttributeInspectorMenu;

    if (auto selected = GetSelected())
    {
        auto* item = static_cast<Serializable*>(args[P_SERIALIZABLE].GetPtr());
        auto* info = static_cast<AttributeInfo*>(args[P_ATTRIBUTEINFO].GetVoidPtr());

        Variant value = item->GetAttribute(info->name_);
        XMLElement styleAttribute;
        XMLElement styleXml;
        Variant styleVariant;
        GetStyleData(*info, styleXml, styleAttribute, styleVariant);

        if (styleVariant != value)
        {
            if (styleXml.NotNull())
            {
                if (ui::MenuItem("Save to style"))
                {
                    if (styleAttribute.IsNull())
                    {
                        styleAttribute = styleXml.CreateChild("attribute");
                        styleAttribute.SetAttribute("name", info->name_);
                    }
                    // To save some writing undo system performs value update action as well.
                    undo_.Add<UndoEditUIStyle>(selected, styleAttribute, value);
                }
            }
        }

        if (styleAttribute.NotNull() && !styleVariant.IsEmpty())
        {
            if (ui::MenuItem("Remove from style"))
            {
                // To save some writing undo system performs value update action as well. Empty variant means removal.
                undo_.Add<UndoEditUIStyle>(selected, styleAttribute, Variant::EMPTY);
            }
        }

        if (info->type_ == VAR_INTRECT && selected->IsInstanceOf<BorderImage>())
        {
            if (ui::MenuItem("Select in UI Texture"))
                textureSelectorAttribute_ = info->name_;
        }
    }
}

void UITab::AttributeCustomize(VariantMap& args)
{
    if (GetSelected() == nullptr)
        return;

    using namespace AttributeInspectorAttribute;

    auto* item = static_cast<Serializable*>(args[P_SERIALIZABLE].GetPtr());
    auto* info = static_cast<AttributeInfo*>(args[P_ATTRIBUTEINFO].GetVoidPtr());

    Variant value = item->GetAttribute(info->name_);
    XMLElement styleAttribute;
    XMLElement styleXml;
    Variant styleVariant;
    GetStyleData(*info, styleXml, styleAttribute, styleVariant);

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

void UITab::OnFocused()
{
    auto* editor = GetSubsystem<Editor>();
    editor->GetTab<HierarchyTab>()->SetProvider(this);
}

}
