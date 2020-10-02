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

#include "../Core/Context.h"
#include "../IO/Log.h"
#include "../Resource/BinaryFile.h"
#include "../RmlUI/RmlUIComponent.h"
#include "../RmlUI/RmlUI.h"

#include "../DebugNew.h"

namespace Urho3D
{

extern const char* RML_UI_CATEGORY;

RmlUIComponent::RmlUIComponent(Context* context)
    : Component(context)
{
    RmlUI* ui = GetSubsystem<RmlUI>();
    ui->documentClosedEvent_.Subscribe(this, &RmlUIComponent::OnDocumentClosed);
    ui->canvasResizedEvent_.Subscribe(this, &RmlUIComponent::OnUICanvasResized);
}

RmlUIComponent::~RmlUIComponent()
{
    CloseInternal();
}

void RmlUIComponent::RegisterObject(Context* context)
{
    context->RegisterFactory<RmlUIComponent>(RML_UI_CATEGORY);
    URHO3D_COPY_BASE_ATTRIBUTES(BaseClassName);
    URHO3D_ACCESSOR_ATTRIBUTE("Resource", GetResource, SetResource, ResourceRef, ResourceRef{BinaryFile::GetTypeStatic()}, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Use Normalized Coordinates", bool, useNormalized_, OnUseNormalizedCoordinates, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Position", GetPosition, SetPosition, Vector2, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Size", GetSize, SetSize, Vector2, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Open", IsOpen, SetOpen, bool, false, AM_DEFAULT);
}

void RmlUIComponent::OnNodeSet(Node* node)
{
    if (open_ && node != nullptr)
        OpenInternal();
    else
        CloseInternal();
}

void RmlUIComponent::SetResource(const ResourceRef& resource)
{
    resource_ = resource;
    if (resource_.type_ == StringHash::ZERO)
        resource_.type_ = BinaryFile::GetTypeStatic();
    SetOpen(open_);
}

void RmlUIComponent::SetOpen(bool open)
{
    if (!open)
        CloseInternal();
    else if (node_ != nullptr  && !resource_.name_.empty())
    {
        OpenInternal();
        open = document_ != nullptr;
    }
    open_ = open;
}

void RmlUIComponent::OpenInternal()
{
    if (document_ != nullptr)
        return;

    if (resource_.name_.empty())
    {
        URHO3D_LOGERROR("UI document can be opened after setting resource path.");
        return;
    }

    RmlUI* ui = GetSubsystem<RmlUI>();
    document_ = ui->LoadDocument(resource_.name_);
    SetPosition(position_);
    SetSize(size_);
    document_->Show();
}

void RmlUIComponent::CloseInternal()
{
    if (document_ == nullptr)
        return;

    position_ = GetPosition();
    size_ = GetSize();
    document_->Close();
    document_ = nullptr;
}

void RmlUIComponent::OnDocumentClosed(Rml::ElementDocument*& document)
{
    if (document_ == document)
    {
        document_ = nullptr;
        open_ = false;
    }
}

void RmlUIComponent::OnUseNormalizedCoordinates()
{

}

Vector2 RmlUIComponent::GetPosition() const
{
    if (document_ == nullptr)
        return position_;

    Vector2 pos = document_->GetAbsoluteOffset(Rml::Box::BORDER);
    if (useNormalized_)
    {
        IntVector2 canvasSize = document_->GetContext()->GetDimensions();
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

    if (useNormalized_)
    {
        IntVector2 canvasSize = document_->GetContext()->GetDimensions();
        pos.x_ = Round(static_cast<float>(canvasSize.x_) * pos.x_);
        pos.y_ = Round(static_cast<float>(canvasSize.y_) * pos.y_);
    }
    document_->SetProperty(Rml::PropertyId::Left, Rml::Property(pos.x_, Rml::Property::PX));
    document_->SetProperty(Rml::PropertyId::Top, Rml::Property(pos.y_, Rml::Property::PX));
    document_->UpdateDocument();
}

Vector2 RmlUIComponent::GetSize() const
{
    if (document_ == nullptr)
        return size_;

    Vector2 size = document_->GetBox().GetSize(Rml::Box::CONTENT);
    if (useNormalized_)
    {
        IntVector2 canvasSize = document_->GetContext()->GetDimensions();
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

    if (useNormalized_)
    {
        IntVector2 canvasSize = document_->GetContext()->GetDimensions();
        size.x_ = Round(static_cast<float>(canvasSize.x_) * size.x_);
        size.y_ = Round(static_cast<float>(canvasSize.y_) * size.y_);
    }
    document_->SetProperty(Rml::PropertyId::Width, Rml::Property(size.x_, Rml::Property::PX));
    document_->SetProperty(Rml::PropertyId::Height, Rml::Property(size.y_, Rml::Property::PX));
    document_->UpdateDocument();
}

void RmlUIComponent::OnUICanvasResized(RmlUICanvasResizedArgs& args)
{
    if (!useNormalized_)
        // Element is positioned using absolute pixel values. Nothing to adjust.
        return;

    // When using normalized coordinates, element is positioned relative to canvas size. When canvas is resized old positions are no longer
    // valid. Convert pixel size and position back to normalized coordiantes using old size and reapply them, which will use new canvas size
    // for calculating new pixel position and size.

    Vector2 pos = document_->GetAbsoluteOffset(Rml::Box::BORDER);
    Vector2 size = document_->GetBox().GetSize(Rml::Box::CONTENT);

    pos.x_ = 1.0f / static_cast<float>(args.oldSize_.x_) * pos.x_;
    pos.y_ = 1.0f / static_cast<float>(args.oldSize_.y_) * pos.y_;
    size.x_ = 1.0f / static_cast<float>(args.oldSize_.x_) * size.x_;
    size.y_ = 1.0f / static_cast<float>(args.oldSize_.y_) * size.y_;

    SetPosition(pos);
    SetSize(size);
}

}
