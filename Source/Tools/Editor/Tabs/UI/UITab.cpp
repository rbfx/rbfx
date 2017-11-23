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

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Toolbox/IO/ContentUtilities.h>
#include <Toolbox/SystemUI/Widgets.h>
#include <IconFontCppHeaders/IconsFontAwesome.h>
#include "Editor/Editor.h"
#include "Editor/Widgets.h"
#include "UITab.h"


namespace Urho3D
{

UITab::UITab(Urho3D::Context* context, Urho3D::StringHash id, const Urho3D::String& afterDockName,
    ui::DockSlot_ position)
    : Tab(context, id, afterDockName, position)
    , undo_(context)
{
    SetTitle("New UI Layout");
    windowFlags_ = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    texture_ = new Texture2D(context);
    texture_->SetFilterMode(FILTER_BILINEAR);
    texture_->SetAddressMode(COORD_U, ADDRESS_CLAMP);
    texture_->SetAddressMode(COORD_V, ADDRESS_CLAMP);
    texture_->SetNumLevels(1);                                        // No mipmaps

    rootElement_ = new RootUIElement(context);
    rootElement_->SetRenderTexture(texture_);
    rootElement_->SetEnabled(true);

    // Prevents crashes due to uninitialized texture.
    SetSize({0, 0, 512, 512});

    undo_.Connect(rootElement_);
    undo_.Connect(&inspector_);

    SubscribeToEvent(E_ATTRIBUTEINSPECTORMENU, std::bind(&UITab::AttributeMenu, this, _2));
    SubscribeToEvent(E_ATTRIBUTEINSPECTOATTRIBUTE, std::bind(&UITab::AttributeCustomize, this, _2));

    AutoLoadDefaultStyle();
}

void UITab::RenderNodeTree()
{
    auto oldSpacing = ui::GetStyle().IndentSpacing;
    ui::GetStyle().IndentSpacing = 10;
    RenderNodeTree(rootElement_);
    ui::GetStyle().IndentSpacing = oldSpacing;
}

void UITab::RenderNodeTree(UIElement* element)
{
    WeakPtr<UIElement> elementRef(element);
    String name = element->GetName();
    String type = element->GetTypeName();
    String tooltip = "Type: " + type;
    if (name.Empty())
        name = type;
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    bool isInternal = element->IsInternal();
    if (isInternal && !showInternal_)
        return;
    else
        flags |= ImGuiTreeNodeFlags_DefaultOpen;

    if (showInternal_)
        tooltip += String("\nInternal: ") + (isInternal ? "true" : "false");

    if (element == selectedElement_)
        flags |= ImGuiTreeNodeFlags_Selected;

    ui::Image(element->GetTypeName());
    ui::SameLine();

    if (ui::TreeNodeEx(element, flags, "%s", name.CString()))
    {
        if (ui::IsItemHovered())
            ui::SetTooltip("%s", tooltip.CString());

        if (ui::IsItemHovered())
        {
            if (ui::IsMouseClicked(0) || ui::IsMouseClicked(2))
            {
                SelectItem(element);
                if (ui::IsMouseClicked(2))
                    ui::OpenPopup("Element Context Menu");
            }
        }

        RenderElementContextMenu();

        // Context menu may delete this element
        bool wasDeleted = (flags & ImGuiTreeNodeFlags_Selected) && selectedElement_.Null();
        if (!wasDeleted)
        {
            // Do not use element->GetChildren() because child may be deleted during this loop.
            PODVector<UIElement*> children;
            element->GetChildren(children);
            for (const auto& child: children)
                RenderNodeTree(child);
        }

        ui::TreePop();
    }
}

void UITab::RenderInspector()
{
    if (auto selected = GetSelected())
        inspector_.RenderAttributes(selected);
}

bool UITab::RenderWindowContent()
{
    auto& style = ui::GetStyle();
    ui::SetCursorPos(ui::GetCursorPos() - style.WindowPadding);
    ui::Image(texture_, ToImGui(tabRect_.Size()));

    if (auto selected = GetSelected())
    {
        // Render element selection rect, resize handles, and handle element transformations.
        IntRect delta;
        IntRect screenRect(selected->GetScreenPosition() + tabRect_.Min(), selected->GetScreenPosition() + selected->GetSize() + tabRect_.Min());
        auto flags = ui::TSF_NONE;
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
        State* s = ui::GetUIState<State>();

        if (ui::TransformRect(screenRect, delta, flags))
        {
            if (!s->resizeActive_)
            {
                s->resizeActive_ = true;
                s->resizeStartPos_ = selected->GetPosition();
                s->resizeStartSize_ = selected->GetSize();
            }
            selected->SetPosition(selected->GetPosition() + delta.Min());
            selected->SetSize(selected->GetSize() + delta.Size());
        }

        if (s->resizeActive_ && !ui::IsItemActive())
        {
            s->resizeActive_ = false;
            undo_.TrackState(selected, "Position", selected->GetPosition(), s->resizeStartPos_);
            undo_.TrackState(selected, "Size", selected->GetSize(), s->resizeStartSize_);
        }
    }

    RenderRectSelector();

    return true;
}

void UITab::RenderToolbarButtons()
{
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

}

void UITab::OnActiveUpdate()
{
    Input* input = GetSubsystem<Input>();
    if (!ui::IsAnyItemActive())
    {
        if (input->GetKeyDown(KEY_CTRL))
        {
            if (input->GetKeyPress(KEY_Y) || (input->GetKeyDown(KEY_SHIFT) && input->GetKeyPress(KEY_Z)))
                undo_.Redo();
            else if (input->GetKeyPress(KEY_Z))
                undo_.Undo();
        }

        if (auto selected = GetSelected())
        {
            if (input->GetKeyPress(KEY_DELETE))
            {
                selected->Remove();
                SelectItem(nullptr);    // Undo system still holds a reference to removed element therefore we must
                                        // manually clear selectedElement_
            }
        }
    }

    if (!ui::IsAnyItemActive() && !ui::IsAnyItemHovered())
    {
        if (input->GetMouseButtonPress(MOUSEB_LEFT) || input->GetMouseButtonPress(MOUSEB_RIGHT))
        {
            auto pos = input->GetMousePosition();
            auto clicked = GetSubsystem<UI>()->GetElementAt(pos, false);
            if (!clicked && rootElement_->GetCombinedScreenRect().IsInside(pos) == INSIDE && !ui::IsAnyWindowHovered())
                clicked = rootElement_;

            if (clicked)
            {
                SelectItem(clicked);

                if (input->GetMouseButtonPress(MOUSEB_RIGHT))
                    ui::OpenPopup("Element Context Menu");
            }
        }
    }

    RenderElementContextMenu();
}

void UITab::SetSize(const IntRect& rect)
{
    if (texture_->SetSize(rect.Width(), rect.Height(), GetSubsystem<Graphics>()->GetRGBAFormat(), TEXTURE_RENDERTARGET))
    {
        Tab::SetSize(rect);
        rootElement_->SetSize(rect.Width(), rect.Height());
        rootElement_->SetOffset(rect.Min());
        texture_->GetRenderSurface()->SetUpdateMode(SURFACE_UPDATEALWAYS);
    }
    else
        URHO3D_LOGERROR("UITab: resizing texture failed.");
}

void UITab::LoadResource(const String& resourcePath)
{
    if (GetContentType(resourcePath) != CTYPE_UILAYOUT)
    {
        URHO3D_LOGERRORF("%s is not a UI layout.", resourcePath.CString());
        return;
    }

    auto cache = GetSubsystem<ResourceCache>();

    SharedPtr<XMLFile> xml(new XMLFile(context_));
    if (xml->Load(*cache->GetFile(resourcePath)))
    {
        Vector<SharedPtr<UIElement>> children = rootElement_->GetChildren();
        auto child = rootElement_->CreateChild(xml->GetRoot().GetAttribute("type"));
        if (child->LoadXML(xml->GetRoot()))
        {
            child->SetStyleAuto();
            SetTitle(GetFileName(resourcePath));

            // Must be disabled because it interferes with ui element resizing
            if (auto window = dynamic_cast<Window*>(child))
            {
                window->SetMovable(false);
                window->SetResizable(false);
            }

            path_ = resourcePath;

            for (const auto& oldChild : children)
                oldChild->Remove();

            undo_.Clear();
        }
        else
        {
            child->Remove();
            URHO3D_LOGERRORF("Loading UI layout %s failed.", resourcePath.CString());
        }
    }
    else
        URHO3D_LOGERRORF("Loading file %s failed.", resourcePath.CString());
}

bool UITab::SaveResource(const String& resourcePath)
{
    if (rootElement_->GetNumChildren() < 1)
        return false;

    auto styleFile = rootElement_->GetDefaultStyle();
    if (styleFile == nullptr)
        return false;

    ResourceCache* cache = GetSubsystem<ResourceCache>();

    String savePath = cache->GetResourceFileName(resourcePath.Empty() ? path_ : resourcePath);
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

        // TODO: remove attributes with values matching style
        // TODO: remove attributes with default values

        File saveFile(context_, savePath, FILE_WRITE);
        if (xml.Save(saveFile))
        {
            if (!resourcePath.Empty())
                path_ = resourcePath;
        }
        else
            return false;
    }
    else
        return false;

    // Save style
    savePath = cache->GetResourceFileName(styleFile->GetName());
    File saveFile(context_, savePath, FILE_WRITE);
    if (!styleFile->Save(saveFile))
        return false;

    if (!path_.Empty())
        SetTitle(GetFileName(path_));

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
        textureSelectorAttribute_.Clear();

    selectedElement_ = current;
}

void UITab::AutoLoadDefaultStyle()
{
    styleNames_.Clear();
    auto cache = GetSubsystem<ResourceCache>();
    auto fs = GetSubsystem<FileSystem>();
    for (const auto& dir: cache->GetResourceDirs())
    {
        Vector<String> items;
        fs->ScanDir(items, dir + "UI", "", SCAN_FILES, false);

        for (const auto& fileName : items)
        {
            auto resourcePath = dir + "UI/" + fileName;
            // Icons file is also a style file. Without this ugly workaround sometimes wrong style gets applied.
            if (GetContentType(resourcePath) == CTYPE_UISTYLE && !resourcePath.EndsWith("Icons.xml"))
            {
                XMLFile* style = cache->GetResource<XMLFile>(resourcePath);
                rootElement_->SetDefaultStyle(style);

                auto styles = style->GetRoot().SelectPrepared(XPathQuery("/elements/element"));
                for (auto i = 0; i < styles.Size(); i++)
                {
                    auto type = styles[i].GetAttribute("type");
                    if (type.Length() && !styleNames_.Contains(type) &&
                        styles[i].GetAttribute("auto").ToLower() == "false")
                        styleNames_.Push(type);
                }
                break;
            }
        }
    }
    Sort(styleNames_.Begin(), styleNames_.End());
}

void UITab::RenderElementContextMenu()
{
    if (ui::BeginPopup("Element Context Menu"))
    {
        if (ui::BeginMenu("Create Child"))
        {
            auto components = GetSubsystem<Editor>()->GetObjectsByCategory("UI");
            Sort(components.Begin(), components.End());

            for (const String& component : components)
            {
                // TODO: element creation with custom styles more usable.
                if (GetSubsystem<Input>()->GetKeyDown(KEY_SHIFT))
                {
                    ui::Image(component);
                    ui::SameLine();
                    if (ui::BeginMenu(component.CString()))
                    {
                        for (auto j = 0; j < styleNames_.Size(); j++)
                        {
                            if (ui::MenuItem(styleNames_[j].CString()))
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
                    if (ui::MenuItem(component.CString()))
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

void UITab::SaveProject(XMLElement& tab)
{
    tab.SetAttribute("type", "ui");
    tab.SetAttribute("id", id_.ToString().CString());
    tab.SetAttribute("path", path_);
    Tab::SaveResource();
}

void UITab::LoadProject(XMLElement& tab)
{
    id_ = StringHash(ToUInt(tab.GetAttribute("id"), 16));
    LoadResource(tab.GetAttribute("path"));
}

String UITab::GetAppliedStyle(UIElement* element)
{
    if (element == nullptr)
        element = selectedElement_;

    if (element == nullptr)
        return "";

    auto appliedStyle = selectedElement_->GetAppliedStyle();
    if (appliedStyle.Empty())
        appliedStyle = selectedElement_->GetTypeName();
    return appliedStyle;
}

void UITab::RenderRectSelector()
{
    BorderImage* selected = dynamic_cast<BorderImage*>(GetSelected());

    if (textureSelectorAttribute_.Empty() || selected == nullptr)
        return;

    struct State
    {
        bool isResizing_ = false;
        IntRect startRect_;
        int textureScale_ = 1;
        int windowFlags_ = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
        IntRect rectWindowDeltaAccumulator_;
    };
    State* s = ui::GetUIState<State>();

    bool open = true;
    auto texture = selected->GetTexture();
    texture->SetFilterMode(FILTER_NEAREST);    // Texture is better visible this way when zoomed in.
    auto padding = ImGui::GetStyle().WindowPadding;
    ui::SetNextWindowPos(ImVec2(texture->GetWidth() + padding.x * 2, texture->GetHeight() + padding.y * 2),
        ImGuiCond_FirstUseEver);
    if (ui::Begin("Select Rect", &open, s->windowFlags_))
    {
        ui::SliderInt("Zoom", &s->textureScale_, 1, 5);
        auto windowPos = ui::GetWindowPos();
        auto imagePos = ui::GetCursorPos();
        ui::Image(texture, ImVec2(texture->GetWidth() * s->textureScale_,
            texture->GetHeight() * s->textureScale_));

        // Disable dragging of window if mouse is hovering texture.
        if (ui::IsItemHovered())
            s->windowFlags_ |= ImGuiWindowFlags_NoMove;
        else
            s->windowFlags_ &= ~ImGuiWindowFlags_NoMove;

        IntRect rect = selectedElement_->GetAttribute(textureSelectorAttribute_).GetIntRect();
        IntRect originalRect = rect;
        // Upscale selection rect if texture is upscaled.
        rect *= s->textureScale_;

        ui::TransformSelectorFlags flags = ui::TSF_NONE;
        if (hideResizeHandles_)
            flags |= ui::TSF_HIDEHANDLES;

        IntRect screenRect(
            rect.Min() + ToIntVector2(imagePos) + ToIntVector2(windowPos),
            IntVector2(rect.right_ - rect.left_, rect.bottom_ - rect.top_)
        );
        // Essentially screenRect().Max() += screenRect().Min()
        screenRect.bottom_ += screenRect.top_;
        screenRect.right_ += screenRect.left_;

        IntRect delta;
        if (ui::TransformRect(screenRect, delta, flags))
        {
            if (!s->isResizing_)
            {
                s->isResizing_ = true;
                s->startRect_ = originalRect;
            }
            // Accumulate delta value. This is required because resizing upscaled rect does not work
            // with small increments when rect values are integers.
            s->rectWindowDeltaAccumulator_ += delta;
        }

        if (ui::IsItemActive())
        {
            // Downscale and add accumulated delta to the original rect value
            rect = originalRect + s->rectWindowDeltaAccumulator_ / s->textureScale_;

            // If downscaled rect size changed compared to original value - set attribute and
            // reset delta accumulator.
            if (rect != originalRect)
            {
                selectedElement_->SetAttribute(textureSelectorAttribute_, rect);
                // Keep remainder in accumulator, otherwise resizing will cause cursor to drift from
                // the handle over time.
                s->rectWindowDeltaAccumulator_.left_ %= s->textureScale_;
                s->rectWindowDeltaAccumulator_.top_ %= s->textureScale_;
                s->rectWindowDeltaAccumulator_.right_ %= s->textureScale_;
                s->rectWindowDeltaAccumulator_.bottom_ %= s->textureScale_;
            }
        }
        else if (s->isResizing_)
        {
            s->isResizing_ = false;
            undo_.TrackState(selected, textureSelectorAttribute_,
                selectedElement_->GetAttribute(textureSelectorAttribute_), s->startRect_);
        }
    }
    ui::End();

    if (!open)
        textureSelectorAttribute_.Clear();
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
    } while (attribute.IsNull() && !styleName.Empty() && !style.IsNull());


    if (!attribute.IsNull() && attribute.GetAttribute("type") != "None")
        value = GetVariantFromXML(attribute, info);
}

void UITab::AttributeMenu(VariantMap& args)
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
                    undo_.TrackState(item, info->name_, styleVariant, value);
                    item->SetAttribute(info->name_, styleVariant);
                    item->ApplyAttributes();
                }
            }

            if (styleXml.NotNull())
            {
                if (ui::MenuItem("Save to style"))
                {
                    if (styleAttribute.IsNull())
                    {
                        styleAttribute = undo_.XMLCreate(styleXml, "attribute");
                        styleAttribute.SetAttribute("name", info->name_);
                        styleAttribute.SetVariantValue(value);
                    }
                    else
                    {
                        undo_.XMLSetVariantValue(styleAttribute, styleAttribute.GetVariantValue(info->type_));
                        undo_.XMLSetVariantValue(styleAttribute, value);
                    }
                }
            }
        }

        if (styleAttribute.NotNull() && !styleVariant.IsEmpty())
        {
            if (ui::MenuItem("Remove from style"))
                undo_.XMLRemove(styleAttribute);
        }

        if (info->type_ == VAR_INTRECT && dynamic_cast<BorderImage*>(selected) != nullptr)
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

    auto* item = dynamic_cast<Serializable*>(args[P_SERIALIZABLE].GetPtr());
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

}
