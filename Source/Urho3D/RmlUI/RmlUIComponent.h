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

#include "../Scene/LogicComponent.h"

#include <RmlUi/Core/DataModelHandle.h>

#include <EASTL/unordered_set.h>

namespace Rml
{
class ElementDocument;
}

namespace Urho3D
{

class RmlCanvasComponent;
class RmlNavigationManager;
struct RmlCanvasResizedArgs;
struct RmlDocumentReloadedArgs;

class RmlUI;

/// Adds a single window to game screen.
class URHO3D_API RmlUIComponent : public LogicComponent
{
    URHO3D_OBJECT(RmlUIComponent, LogicComponent);

    using GetterFunc = eastl::function<void(Variant&)>;
    using SetterFunc = eastl::function<void(const Variant&)>;
    using EventFunc = eastl::function<void(const VariantVector&)>;

public:
    /// Construct.
    explicit RmlUIComponent(Context* context);
    /// Destruct.
    ~RmlUIComponent() override;

    /// Registers object with the engine.
    static void RegisterObject(Context* context);

    /// Implement LogicComponent.
    /// @{
    void Update(float timeStep) override;
    /// @}

    /// Returns instance of the component from the document.
    static RmlUIComponent* FromDocument(Rml::ElementDocument* document);

    /// Attributes
    /// @{
    void SetResource(const ResourceRef& resourceRef);
    void SetResource(const ea::string& resourceName);
    const ResourceRef& GetResource() const { return resource_; }
    bool GetUseNormalizedCoordinates() const { return useNormalized_; }
    void SetUseNormalizedCoordinates(bool enable) { useNormalized_ = enable; }
    Vector2 GetPosition() const;
    void SetPosition(Vector2 pos);
    Vector2 GetSize() const;
    void SetSize(Vector2 size);
    void SetAutoSize(bool enable) { autoSize_ = enable; }
    bool GetAutoSize() const { return autoSize_; }
    /// @}

    /// Return RmlUI subsystem this component renders into.
    RmlUI* GetUI() const;
    /// Return currently open document, may be null.
    Rml::ElementDocument* GetDocument() const { return document_; }
    /// Return navigation manager.
    RmlNavigationManager& GetNavigationManager() const { return *navigationManager_; }

    // Bind data model property.
    bool BindDataModelProperty(const ea::string& name, GetterFunc getter, SetterFunc setter);
    // Bind data model event.
    bool BindDataModelEvent(const ea::string& name, EventFunc eventCallback);

protected:
    /// Data model facade
    /// @{
    bool IsVariableDirty(const ea::string& variableName)
    {
        if (!dataModel_)
            return false;
        return dataModel_.IsVariableDirty(variableName);
    }
    void DirtyVariable(const ea::string& variableName)
    {
        if (dataModel_)
            dataModel_.DirtyVariable(variableName);
    }
    void DirtyAllVariables()
    {
        if (dataModel_)
            dataModel_.DirtyAllVariables();
    }
    /// @}

    /// Wrap data event callback.
    template <class T>
    Rml::DataEventFunc WrapCallback(void(T::*callback)())
    {
        auto self = static_cast<T*>(this);
        return [self, callback](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
        {
            const bool enabled = args.empty() || args[0].Get<bool>();
            if (enabled)
                (self->*callback)();
        };
    }

    /// Callbacks for document loading and unloading.
    /// If load failed, only first callback will be called.
    /// @{
    virtual ea::string GetDataModelName() { return GetTypeName(); }
    virtual void OnDataModelInitialized() {}

    virtual void OnDocumentPreLoad() {}
    virtual void OnDocumentPostLoad() {}
    virtual void OnDocumentPreUnload() {}
    virtual void OnDocumentPostUnload() {}
    /// @}

    /// Get data model constructor. Only available in OnDataModelInitialized method.
    Rml::DataModelConstructor* GetDataModelConstructor() const { return modelConstructor_.get(); }

    /// Implement Component
    /// @{
    void OnSetEnabled() override;
    void OnNodeSet(Node* previousNode, Node* currentNode) override;
    /// @}
private:

    /// Open a window document if it was not already open.
    void OpenInternal();
    /// Close a window document if it was open.
    void CloseInternal();

    /// Handle subsystem events
    /// @{
    void OnDocumentClosed(Rml::ElementDocument* document);
    void OnUICanvasResized(const RmlCanvasResizedArgs& size);
    void OnDocumentReloaded(const RmlDocumentReloadedArgs& args);
    /// @}

    /// Set currently active document and link it to the component.
    void SetDocument(Rml::ElementDocument* document);
    void UpdateDocumentOpen();
    void UpdateConnectedCanvas();

    void CreateDataModel();
    void RemoveDataModel();

    void OnNavigableGroupChanged();
    void DoNavigablePush(Rml::DataModelHandle model, Rml::Event& event, const Rml::VariantList& args);
    void DoNavigablePop(Rml::DataModelHandle model, Rml::Event& event, const Rml::VariantList& args);

    /// Attributes
    /// @{
    ResourceRef resource_;
    bool useNormalized_ = false;
    Vector2 size_;
    Vector2 position_;
    bool autoSize_ = true;
    /// @}

    /// Navigation manager.
    SharedPtr<RmlNavigationManager> navigationManager_;
    /// Currently open document. Null if document was closed.
    Rml::ElementDocument* document_{};
    /// Component which holds RmlUI instance containing UI managed by this component. May be null if UI is rendered into default RmlUI subsystem.
    WeakPtr<RmlCanvasComponent> canvasComponent_;

    /// Type registry for the data model.
    Rml::DataTypeRegister typeRegister_;
    /// Data model for the document.
    Rml::DataModelHandle dataModel_;
    /// Name of the data model.
    ea::string dataModelName_;

    /// Data model constructor.
    ea::unique_ptr<Rml::DataModelConstructor> modelConstructor_;
};

}
