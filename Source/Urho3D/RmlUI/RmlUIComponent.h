// Copyright (c) 2017-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Scene/LogicComponent.h"

#include <RmlUi/Core/DataModelHandle.h>

#include <EASTL/optional.h>
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

    /// Set current document body element font size in pixels, also known as "em" unit.
    void SetEmSize(float sizePx);
    /// Get current document body element font size, also known as "em" unit.
    float GetEmSize() const;

    /// Return whether the current document is modal.
    bool IsModal() const;
    /// Set modal flag to the current document.
    void SetModal(bool modal);

    /// Focus on the document.
    void Focus(bool focusVisible);

    // Bind data model property.
    bool BindDataModelProperty(const ea::string& name, GetterFunc getter, SetterFunc setter);
    // Bind data model property or Urho3D::Variant type.
    bool BindDataModelVariant(const ea::string& name, Variant* value);
    // Bind data model property or Urho3D::VariantVector type.
    bool BindDataModelVariantVector(const ea::string& name, VariantVector* value);
    // Bind data model property or Urho3D::VariantMap type.
    bool BindDataModelVariantMap(const ea::string& name, VariantMap* value);
    // Bind data model event.
    bool BindDataModelEvent(const ea::string& name, EventFunc eventCallback);

protected:
    /// Data model facade.
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
    template <class T> Rml::DataEventFunc WrapCallback(void (T::*callback)())
    {
        auto self = static_cast<T*>(this);
        return [self, callback](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
        {
            const bool enabled = args.empty() || args[0].Get<bool>();
            if (enabled)
                (self->*callback)();
        };
    }
    template <class T> Rml::DataEventFunc WrapCallback(void (T::*callback)(const Rml::VariantList& args))
    {
        auto self = static_cast<T*>(this);
        return [self, callback](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
        {
            const bool enabled = args.empty() || args[0].Get<bool>();
            if (enabled)
                (self->*callback)(args);
        };
    }

    /// Callbacks for document loading and unloading.
    /// If load failed, only first callback will be called.
    /// @{
    virtual ea::string GetDataModelName() { return "{{__data_model_id}}"; }
    virtual void OnDataModelInitialized() {}

    virtual void OnDocumentPreLoad() {}
    virtual void OnDocumentPostLoad() {}
    virtual void OnDocumentPreUnload() {}
    virtual void OnDocumentPostUnload() {}
    /// @}

    /// Get data model constructor. Only available in OnDataModelInitialized method.
    Rml::DataModelConstructor* GetDataModelConstructor() const { return modelConstructor_.get(); }
    /// If current focus is invalid, focus on the first valid navigable element.
    void RestoreFocus();
    /// Schedule element focus by ID on next update. This is useful when focus-ability is not updated yet.
    void ScheduleFocusById(const ea::string& elementId);

    /// Implement Component
    /// @{
    void OnSetEnabled() override;
    void OnNodeSet(Node* previousNode, Node* currentNode) override;
    /// @}

private:
    /// Get data model constructor. Logs error if the constructor is not available.
    Rml::DataModelConstructor* ExpectDataModelConstructor() const;

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
    void UpdatePendingFocus();

    void CreateDataModel();
    void RemoveDataModel();
    void InsertVariablePlaceholders(ea::string& dataModelName);

    void OnNavigableGroupChanged();
    void DoNavigablePush(Rml::DataModelHandle model, Rml::Event& event, const Rml::VariantList& args);
    void DoNavigablePop(Rml::DataModelHandle model, Rml::Event& event, const Rml::VariantList& args);
    void DoFocusById(Rml::DataModelHandle model, Rml::Event& event, const Rml::VariantList& args);

    /// Attributes
    /// @{
    ResourceRef resource_;
    bool useNormalized_ = false;
    Vector2 size_;
    Vector2 position_;
    bool autoSize_ = true;
    bool modal_ = false;
    /// @}

    /// Navigation manager.
    SharedPtr<RmlNavigationManager> navigationManager_;
    /// Currently open document. Null if document was closed.
    Rml::ElementDocument* document_{};
    /// Component which holds RmlUI instance containing UI managed by this component. May be null if UI is rendered into
    /// default RmlUI subsystem.
    WeakPtr<RmlCanvasComponent> canvasComponent_;

    /// Type registry for the data model.
    ea::optional<Rml::DataTypeRegister> typeRegister_;
    /// Data model for the document.
    Rml::DataModelHandle dataModel_;
    /// Name of the data model.
    ea::string dataModelName_;

    /// Data model constructor.
    ea::unique_ptr<Rml::DataModelConstructor> modelConstructor_;

    /// Element id to be focused on next update.
    ea::optional<ea::string> pendingFocusId_;
    /// Whether to suppress next call to RestoreFocus().
    bool suppressRestoreFocus_{};
};

} // namespace Urho3D
