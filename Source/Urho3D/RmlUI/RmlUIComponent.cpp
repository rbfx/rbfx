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

#include "../Precompiled.h"

#include "../RmlUI/RmlUI.h"

#include "../Core/Context.h"
#include "../Graphics/Material.h"
#include "../IO/Log.h"
#include "../Resource/BinaryFile.h"
#include "../RmlUI/RmlCanvasComponent.h"
#include "../RmlUI/RmlNavigationManager.h"
#include "../RmlUI/RmlUIComponent.h"
#include "../Scene/Node.h"
#include "../Scene/Scene.h"

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

const Rml::String ComponentPtrAttribute = "__RmlUIComponentPtr__";

}

RmlUIComponent::RmlUIComponent(Context* context)
    : LogicComponent(context)
    , navigationManager_(MakeShared<RmlNavigationManager>(this))
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

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Resource", GetResource, SetResource, ResourceRef, ResourceRef{BinaryFile::GetTypeStatic()}, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Use Normalized Coordinates", bool, useNormalized_, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Position", GetPosition, SetPosition, Vector2, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Size", GetSize, SetSize, Vector2, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Auto Size", bool, autoSize_, true, AM_DEFAULT);
}

void RmlUIComponent::Update(float timeStep)
{
    navigationManager_->Update();
    // There should be only a few of RmlUIComponent enabled at a time, so this is not a performance issue.
    UpdateConnectedCanvas();
}

bool RmlUIComponent::BindDataModelProperty(const ea::string& name, GetterFunc getter, SetterFunc setter)
{
    const auto constructor = GetDataModelConstructor();
    if (!constructor)
    {
        URHO3D_LOGERROR("BindDataModelProperty can only be executed from OnDataModelInitialized");
        return false;
    }
    return constructor->BindFunc(
        name,
        [=](Rml::Variant& outputValue)
        {
        Variant variant;
        getter(variant);
        RmlUI::TryConvertVariant(variant, outputValue);
        },
        [=](const Rml::Variant& inputValue)
        {
        Variant variant;
        if (RmlUI::TryConvertVariant(inputValue, variant))
        {
            setter(variant);
        }
    });
}

bool RmlUIComponent::BindDataModelEvent(const ea::string& name, EventFunc eventCallback)
{
    auto constructor = GetDataModelConstructor();
    if (!constructor)
    {
        URHO3D_LOGERROR("BindDataModelProperty can only be executed from OnDataModelInitialized");
        return false;
    }
    return constructor->BindEventCallback(
        name,
        [=](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args)
        {
        VariantVector urhoArgs;
        for (auto& src: args)
        {
            RmlUI::TryConvertVariant(src, urhoArgs.push_back());
        }
        eventCallback(urhoArgs);
    });
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
    document_->Show();
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

    // When using normalized coordinates, element is positioned relative to canvas size. When canvas is resized old positions are no longer
    // valid. Convert pixel size and position back to normalized coordiantes using old size and reapply them, which will use new canvas size
    // for calculating new pixel position and size.

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

void RmlUIComponent::CreateDataModel()
{
    RmlUI* ui = GetUI();
    Rml::Context* context = ui->GetRmlContext();

    dataModelName_ = GetDataModelName();
    modelConstructor_ = ea::make_unique<Rml::DataModelConstructor>(context->CreateDataModel(dataModelName_, &typeRegister_));

    modelConstructor_->BindFunc(
        "navigable_group", [this](Rml::Variant& result) { result = navigationManager_->GetTopCursorGroup(); });
    modelConstructor_->BindEventCallback("navigable_push", &RmlUIComponent::DoNavigablePush, this);
    modelConstructor_->BindEventCallback("navigable_pop", &RmlUIComponent::DoNavigablePop, this);
}

void RmlUIComponent::RemoveDataModel()
{
    RmlUI* ui = GetUI();
    Rml::Context* context = ui->GetRmlContext();
    context->RemoveDataModel(dataModelName_);

    dataModel_ = nullptr;
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

}
