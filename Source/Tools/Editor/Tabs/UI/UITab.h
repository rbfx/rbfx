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


#include <Urho3D/Urho3DAll.h>
#include <Toolbox/Common/UndoManager.h>
#include "Tabs/BaseResourceTab.h"
#include "Tabs/UI/RootUIElement.h"


namespace Urho3D
{

class UITab : public BaseResourceTab, public IHierarchyProvider, public IInspectorProvider
{
    URHO3D_OBJECT(UITab, BaseResourceTab);
public:
    /// Construct.
    explicit UITab(Context* context);
    /// Render scene hierarchy window.
    void RenderHierarchy() override;
    /// Render inspector window.
    void RenderInspector(const char* filter) override;
    /// Render content of tab window.
    bool RenderWindowContent() override;
    /// Render toolbar buttons.
    void RenderToolbarButtons() override;
    /// Update window when it is active.
    void OnActiveUpdate() override;
    /// Save project data to xml.
    void OnSaveProject(JSONValue& tab) override;
    /// Load project data from xml.
    void OnLoadProject(const JSONValue& tab) override;
    /// Load UI layout from resource path.
    bool LoadResource(const String& resourcePath) override;
    /// Save scene to a resource file.
    bool SaveResource() override;
    ///
    StringHash GetResourceType() override { return XMLFile::GetTypeStatic(); };
    /// Called when tab focused.
    void OnFocused() override;
    /// Return selected UIElement.
    UIElement* GetSelected() const;

protected:
    ///
    IntRect UpdateViewRect() override;
    /// Render scene hierarchy window.
    void RenderNodeTree(UIElement* element);
    /// Select element. Pass null to unselect current element.
    void SelectItem(UIElement* current);
    /// Searches resource path for style file in UI directory. First style found is applied. There are no restrictions on style file name.
    void AutoLoadDefaultStyle();
    /// Render element context menu.
    void RenderElementContextMenu();
    ///
    void RenderRectSelector();
    ///
    Variant GetVariantFromXML(const XMLElement& attribute, const AttributeInfo& info) const;
    ///
    String GetAppliedStyle(UIElement* element = nullptr);
    ///
    void GetStyleData(const AttributeInfo& info, XMLElement& style, XMLElement& attribute, Variant& value);
    ///
    void AttributeMenu(VariantMap& args);
    ///
    void AttributeCustomize(VariantMap& args);

    ///
    SharedPtr<UI> offScreenUI_;
    /// Root element which contains edited UI.
    SharedPtr<RootUIElement> rootElement_;
    /// Texture that UIElement will be rendered into.
    SharedPtr<Texture2D> texture_;
    /// Flag enabling display of internal elements.
    bool showInternal_ = false;

    WeakPtr<UIElement> selectedElement_;
    bool hideResizeHandles_ = false;
    Vector<String> styleNames_;
    String textureSelectorAttribute_;
};

}
