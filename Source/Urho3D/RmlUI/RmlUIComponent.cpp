// Copyright (c) 2017-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RmlUI/RmlUIComponent.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/Material.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Resource/BinaryFile.h"
#include "Urho3D/RmlUI/RmlCanvasComponent.h"
#include "Urho3D/RmlUI/RmlNavigationManager.h"
#include "Urho3D/RmlUI/RmlUI.h"
#include "Urho3D/Scene/Node.h"
#include "Urho3D/Scene/Scene.h"

#include <RmlUi/Core/ComputedValues.h>

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

const Rml::String ComponentPtrAttribute = "__RmlUIComponentPtr__";

bool IsElementNavigable(Rml::Element* element)
{
    // The element itself should be tab-able
    if (!element || element->GetComputedValues().tab_index() == Rml::Style::TabIndex::None)
        return false;

    // Entire element hierarchy should be visible and focusable
    while (element)
    {
        if (!element->IsVisible() || element->GetComputedValues().focus() == Rml::Style::Focus::None)
            return false;

        element = element->GetParentNode();
    }

    return true;
};

} // namespace

RmlUIComponent::RmlUIComponent(Context* context)
    : LogicComponent(context)
    , navigationManager_(MakeShared<RmlNavigationManager>(this))
    , resource_{BinaryFile::GetTypeStatic()}
{
    SetUpdateEventMask(USE_UPDATE);
    navigationManager_->OnGroupChanged.Subscribe(this, &RmlUIComponent::OnNavigableGroupChanged);
}

RmlUIComponent::~RmlUIComponent()
{
    CloseInternal();
}

void RmlUIComponent::RegisterObject(Context* context)
{
    context->AddFactoryReflection<RmlUIComponent>(Category_RmlUI);

    // clang-format off
    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Resource", GetResource, SetResource, ResourceRef, ResourceRef{BinaryFile::GetTypeStatic()}, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Use Normalized Coordinates", bool, useNormalized_, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Position", GetPosition, SetPosition, Vector2, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Size", GetSize, SetSize, Vector2, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Auto Size", bool, autoSize_, true, AM_DEFAULT);
    // clang-format on
}

void RmlUIComponent::Update(float timeStep)
{
    navigationManager_->Update();
    // There should be only a few of RmlUIComponent enabled at a time, so this is not a performance issue.
    UpdateConnectedCanvas();
    UpdatePendingFocus();
}

bool RmlUIComponent::BindDataModelProperty(const ea::string& name, GetterFunc getter, SetterFunc setter)
{
    const auto constructor = ExpectDataModelConstructor();
    if (!constructor)
    {
        return false;
    }
    return constructor->BindFunc(name,
        [=](Rml::Variant& outputValue)
    {
        Variant variant;
        getter(variant);
        ToRmlUi(variant, outputValue);
    }, [=](const Rml::Variant& inputValue)
    {
        Variant variant;
        if (FromRmlUi(inputValue, variant))
        {
            setter(variant);
        }
    });
}

bool RmlUIComponent::BindDataModelVariant(const ea::string& name, Variant* value)
{
    const auto constructor = ExpectDataModelConstructor();
    if (!constructor || !typeRegister_)
    {
        return false;
    }
    return constructor->BindCustomDataVariable(name, {typeRegister_->GetDefinition<Variant>(), value});
}

bool RmlUIComponent::BindDataModelVariantVector(const ea::string& name, VariantVector* value)
{
    const auto constructor = ExpectDataModelConstructor();
    if (!constructor || !typeRegister_)
    {
        return false;
    }
    return constructor->BindCustomDataVariable(name, {typeRegister_->GetDefinition<VariantVector>(), value});
}

bool RmlUIComponent::BindDataModelVariantMap(const ea::string& name, VariantMap* value)
{
    const auto constructor = ExpectDataModelConstructor();
    if (!constructor || !typeRegister_)
    {
        return false;
    }
    return constructor->BindCustomDataVariable(name, {typeRegister_->GetDefinition<VariantMap>(), value});
}

bool RmlUIComponent::BindDataModelEvent(const ea::string& name, EventFunc eventCallback)
{
    const auto constructor = ExpectDataModelConstructor();
    if (!constructor)
    {
        return false;
    }
    return constructor->BindEventCallback(name, [=](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
    {
        VariantVector urhoArgs;
        for (auto& src : args)
        {
            FromRmlUi(src, urhoArgs.push_back());
        }
        eventCallback(urhoArgs);
    });
}

Rml::DataModelConstructor* RmlUIComponent::ExpectDataModelConstructor() const
{
    const auto constructor = GetDataModelConstructor();
    if (!constructor)
    {
        URHO3D_LOGERROR("BindDataModelProperty can only be executed from OnDataModelInitialized");
    }
    return constructor;
}

void RmlUIComponent::UpdatePendingFocus()
{
    suppressRestoreFocus_ = false;

    if (pendingFocusId_ && document_)
    {
        if (Rml::Element* element = document_->GetElementById(*pendingFocusId_))
        {
            if (element->Focus(true))
            {
                element->ScrollIntoView(Rml::ScrollAlignment::Nearest);
                suppressRestoreFocus_ = true;
            }
        }
        pendingFocusId_ = ea::nullopt;
    }
}

void RmlUIComponent::RestoreFocus()
{
    if (document_ && document_->IsVisible() && !suppressRestoreFocus_
        && !IsElementNavigable(document_->GetFocusLeafNode()))
    {
        if (Rml::Element* nextElement = document_->FindNextTabElement(document_, true))
        {
            if (nextElement->Focus(true))
            {
                nextElement->ScrollIntoView(Rml::ScrollAlignment::Nearest);
            }
        }
    }
}

void RmlUIComponent::ScheduleFocusById(const ea::string& elementId)
{
    pendingFocusId_ = elementId;
}

void RmlUIComponent::OnSetEnabled()
{
    UpdateDocumentOpen();
}

void RmlUIComponent::OnNodeSet(Node* previousNode, Node* currentNode)
{
    if (node_)
        UpdateConnectedCanvas();
    UpdateDocumentOpen();
}

void RmlUIComponent::SetResource(const ResourceRef& resourceRef)
{
    resource_ = resourceRef;
    if (resource_.type_ == StringHash::Empty)
        resource_.type_ = BinaryFile::GetTypeStatic();
    UpdateDocumentOpen();
}

void RmlUIComponent::OpenInternal()
{
    if (document_ != nullptr)
        return; // Already open.

    if (resource_.name_.empty())
    {
        URHO3D_LOGERROR("UI document can be opened after setting resource path.");
        return;
    }

    RmlUI* ui = GetUI();
    ui->documentClosedEvent_.Subscribe(this, &RmlUIComponent::OnDocumentClosed);
    ui->canvasResizedEvent_.Subscribe(this, &RmlUIComponent::OnUICanvasResized);
    ui->documentReloaded_.Subscribe(this, &RmlUIComponent::OnDocumentReloaded);

    OnDocumentPreLoad();

    if (!dataModel_)
    {
        CreateDataModel();
        OnDataModelInitialized();
        dataModel_ = modelConstructor_->GetModelHandle();
        modelConstructor_.reset();
    }

    SetDocument(ui->LoadDocument(resource_.name_));

    if (document_ == nullptr)
    {
        URHO3D_LOGERROR("Failed to load UI document: {}", resource_.name_);
        return;
    }

    SetPosition(position_);
    SetSize(size_);
    document_->Show(modal_ ? Rml::ModalFlag::Modal : Rml::ModalFlag::None, Rml::FocusFlag::None);
    OnDocumentPostLoad();
}

void RmlUIComponent::CloseInternal()
{
    if (document_ == nullptr)
        return; // Already closed.

    RmlUI* ui = static_cast<Detail::RmlContext*>(document_->GetContext())->GetOwnerSubsystem();
    ui->documentClosedEvent_.Unsubscribe(this);
    ui->canvasResizedEvent_.Unsubscribe(this);
    ui->documentReloaded_.Unsubscribe(this);

    position_ = GetPosition();
    size_ = GetSize();

    OnDocumentPreUnload();
    document_->Close();
    SetDocument(nullptr);

    if (dataModel_)
        RemoveDataModel();

    OnDocumentPostUnload();
}

void RmlUIComponent::OnDocumentClosed(Rml::ElementDocument* document)
{
    if (document_ == document)
    {
        SetDocument(nullptr);
    }
}

Vector2 RmlUIComponent::GetPosition() const
{
    if (document_ == nullptr)
        return position_;

    Vector2 pos = ToVector2(document_->GetAbsoluteOffset(Rml::BoxArea::Border));
    if (useNormalized_)
    {
        IntVector2 canvasSize = ToIntVector2(document_->GetContext()->GetDimensions());
        pos.x_ = 1.0f / static_cast<float>(canvasSize.x_) * pos.x_;
        pos.y_ = 1.0f / static_cast<float>(canvasSize.y_) * pos.y_;
    }
    return pos;
}

void RmlUIComponent::SetPosition(Vector2 pos)
{
    if (document_ == nullptr)
    {
        position_ = pos;
        return;
    }

    if (pos == Vector2::ZERO)
        return;

    if (useNormalized_)
    {
        IntVector2 canvasSize = ToIntVector2(document_->GetContext()->GetDimensions());
        pos.x_ = Round(static_cast<float>(canvasSize.x_) * pos.x_);
        pos.y_ = Round(static_cast<float>(canvasSize.y_) * pos.y_);
    }
    document_->SetProperty(Rml::PropertyId::Left, Rml::Property(pos.x_, Rml::Unit::PX));
    document_->SetProperty(Rml::PropertyId::Top, Rml::Property(pos.y_, Rml::Unit::PX));
    document_->UpdateDocument();
}

Vector2 RmlUIComponent::GetSize() const
{
    if (document_ == nullptr)
        return size_;

    if (autoSize_)
        return Vector2::ZERO;

    Vector2 size = ToVector2(document_->GetBox().GetSize(Rml::BoxArea::Content));
    if (useNormalized_)
    {
        IntVector2 canvasSize = ToIntVector2(document_->GetContext()->GetDimensions());
        size.x_ = 1.0f / static_cast<float>(canvasSize.x_) * size.x_;
        size.y_ = 1.0f / static_cast<float>(canvasSize.y_) * size.y_;
    }
    return size;
}

void RmlUIComponent::SetSize(Vector2 size)
{
    if (document_ == nullptr)
    {
        size_ = size;
        return;
    }

    if (size == Vector2::ZERO || autoSize_)
        return;

    if (useNormalized_)
    {
        IntVector2 canvasSize = ToIntVector2(document_->GetContext()->GetDimensions());
        size.x_ = Round(static_cast<float>(canvasSize.x_) * size.x_);
        size.y_ = Round(static_cast<float>(canvasSize.y_) * size.y_);
    }
    document_->SetProperty(Rml::PropertyId::Width, Rml::Property(size.x_, Rml::Unit::PX));
    document_->SetProperty(Rml::PropertyId::Height, Rml::Property(size.y_, Rml::Unit::PX));
    document_->UpdateDocument();
}

void RmlUIComponent::OnUICanvasResized(const RmlCanvasResizedArgs& args)
{
    if (!useNormalized_)
        // Element is positioned using absolute pixel values. Nothing to adjust.
        return;

    // When using normalized coordinates, element is positioned relative to canvas size. When canvas is resized old
    // positions are no longer valid. Convert pixel size and position back to normalized coordiantes using old size and
    // reapply them, which will use new canvas size for calculating new pixel position and size.

    Vector2 pos = ToVector2(document_->GetAbsoluteOffset(Rml::BoxArea::Border));
    Vector2 size = ToVector2(document_->GetBox().GetSize(Rml::BoxArea::Content));

    pos.x_ = 1.0f / static_cast<float>(args.oldSize_.x_) * pos.x_;
    pos.y_ = 1.0f / static_cast<float>(args.oldSize_.y_) * pos.y_;
    size.x_ = 1.0f / static_cast<float>(args.oldSize_.x_) * size.x_;
    size.y_ = 1.0f / static_cast<float>(args.oldSize_.y_) * size.y_;

    SetPosition(pos);
    SetSize(size);
}

void RmlUIComponent::OnDocumentReloaded(const RmlDocumentReloadedArgs& args)
{
    if (document_ == args.unloadedDocument_)
        SetDocument(args.loadedDocument_);
}

void RmlUIComponent::SetResource(const eastl::string& resourceName)
{
    ResourceRef resourceRef(BinaryFile::GetTypeStatic(), resourceName);
    SetResource(resourceRef);
}

RmlUI* RmlUIComponent::GetUI() const
{
    if (canvasComponent_ != nullptr)
        return canvasComponent_->GetUI();
    return GetSubsystem<RmlUI>();
}

void RmlUIComponent::SetEmSize(float sizePx)
{
    if (!document_)
    {
        return;
    }

    document_->SetProperty(Rml::PropertyId::FontSize, Rml::Property(sizePx, Rml::Unit::PX));
}

float RmlUIComponent::GetEmSize() const
{
    if (!document_)
    {
        return 0.0f;
    }

    return document_->ResolveLength(document_->GetProperty(Rml::PropertyId::FontSize)->GetNumericValue());
}

bool RmlUIComponent::IsModal() const
{
    return modal_;
}

void RmlUIComponent::SetModal(bool modal)
{
    if (modal_ == modal)
        return;

    modal_ = modal;
    if (!document_)
        return;

    if (document_->IsVisible())
    {
        const Rml::Element* oldFocusedElement = document_->GetContext()->GetFocusElement();
        const Rml::FocusFlag focus = oldFocusedElement && oldFocusedElement->GetOwnerDocument() == document_
            ? Rml::FocusFlag::Document
            : Rml::FocusFlag::Auto;

        document_->Hide();
        document_->Show(modal ? Rml::ModalFlag::Modal : Rml::ModalFlag::None, focus);
    }
}

void RmlUIComponent::Focus(bool focusVisible)
{
    if (document_)
    {
        document_->Focus(focusVisible);
    }
}

void RmlUIComponent::SetDocument(Rml::ElementDocument* document)
{
    if (document_ != document)
    {
        if (document_)
            document_->SetAttribute(ComponentPtrAttribute, static_cast<void*>(nullptr));

        document_ = document;

        if (document_)
        {
            document_->SetAttribute(ComponentPtrAttribute, static_cast<void*>(this));
            navigationManager_->Reset(document_);
        }
    }
}

void RmlUIComponent::OnNavigableGroupChanged()
{
    DirtyVariable("navigable_group");
}

void RmlUIComponent::DoNavigablePush(Rml::DataModelHandle model, Rml::Event& event, const Rml::VariantList& args)
{
    if (args.size() > 2)
    {
        URHO3D_LOGWARNING("RmlUIComponent::DoNavigablePush is called with unexpected arguments");
        return;
    }

    const bool enabled = args.size() > 1 ? args[0].Get<bool>() : true;
    const ea::string group = args.back().Get<Rml::String>();
    if (enabled)
        navigationManager_->PushCursorGroup(group);
}

void RmlUIComponent::DoNavigablePop(Rml::DataModelHandle model, Rml::Event& event, const Rml::VariantList& args)
{
    if (args.size() > 1)
    {
        URHO3D_LOGWARNING("RmlUIComponent::DoNavigablePop is called with unexpected arguments");
        return;
    }

    const bool enabled = args.size() > 0 ? args[0].Get<bool>() : true;
    if (enabled)
        navigationManager_->PopCursorGroup();
}

void RmlUIComponent::DoFocusById(Rml::DataModelHandle model, Rml::Event& event, const Rml::VariantList& args)
{
    if (args.size() != 2)
    {
        URHO3D_LOGWARNING("RmlUIComponent::DoFocusById is called with unexpected arguments");
        return;
    }

    const bool enabled = args.size() > 0 ? args[0].Get<bool>() : true;
    const auto& id = args[1].Get<Rml::String>();
    if (enabled)
        ScheduleFocusById(id);
}

void RmlUIComponent::CreateDataModel()
{
    RmlUI* ui = GetUI();
    Rml::Context* context = ui->GetRmlContext();

    dataModelName_ = GetDataModelName();
    typeRegister_.emplace();
    modelConstructor_ =
        ea::make_unique<Rml::DataModelConstructor>(context->CreateDataModel(dataModelName_, &*typeRegister_));
    RegisterVariantDefinition(&*typeRegister_);

    modelConstructor_->BindFunc(
        "navigable_group", [this](Rml::Variant& result) { result = navigationManager_->GetTopCursorGroup(); });
    modelConstructor_->BindEventCallback("navigable_push", &RmlUIComponent::DoNavigablePush, this);
    modelConstructor_->BindEventCallback("navigable_pop", &RmlUIComponent::DoNavigablePop, this);
    modelConstructor_->BindEventCallback("focus", &RmlUIComponent::DoFocusById, this);
}

void RmlUIComponent::RemoveDataModel()
{
    RmlUI* ui = GetUI();
    Rml::Context* context = ui->GetRmlContext();
    context->RemoveDataModel(dataModelName_);

    dataModel_ = nullptr;
    typeRegister_ = ea::nullopt;
    dataModelName_.clear();
}

void RmlUIComponent::UpdateDocumentOpen()
{
    const bool shouldBeOpen = IsEnabledEffective() && !resource_.name_.empty();
    const bool isOpen = document_ != nullptr;

    if (shouldBeOpen && !isOpen)
        OpenInternal();
    else if (!shouldBeOpen && isOpen)
        CloseInternal();
}

void RmlUIComponent::UpdateConnectedCanvas()
{
    RmlCanvasComponent* newCanvasComponent = IsEnabledEffective() ? node_->GetComponent<RmlCanvasComponent>() : nullptr;
    if (canvasComponent_ != newCanvasComponent)
    {
        const bool wasOpen = document_ != nullptr;
        CloseInternal();
        canvasComponent_ = newCanvasComponent;
        if (wasOpen)
            OpenInternal();
    }
}

RmlUIComponent* RmlUIComponent::FromDocument(Rml::ElementDocument* document)
{
    if (document)
    {
        if (const Rml::Variant* value = document->GetAttribute(ComponentPtrAttribute))
            return static_cast<RmlUIComponent*>(value->Get<void*>());
    }
    return nullptr;
}

} // namespace Urho3D
