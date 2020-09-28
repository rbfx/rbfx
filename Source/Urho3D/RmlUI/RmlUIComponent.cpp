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
#include "../RmlUI/RmlEvents.h"
#include "../RmlUI/RmlUI.h"

#include "../DebugNew.h"

namespace Urho3D
{

extern const char* RML_UI_CATEGORY;

RmlUIComponent::RmlUIComponent(Context* context)
    : Component(context)
{
    RmlUI* ui = GetSubsystem<RmlUI>();
    SubscribeToEvent(ui, E_UIDOCUMENTCLOSED, &RmlUIComponent::OnDocumentClosed);
}

RmlUIComponent::~RmlUIComponent()
{
    CloseInternal();
}

void RmlUIComponent::RegisterObject(Context* context)
{
    context->RegisterFactory<RmlUIComponent>(RML_UI_CATEGORY);
    URHO3D_COPY_BASE_ATTRIBUTES(BaseClassName);
    URHO3D_ATTRIBUTE_EX("Window Resource", ResourceRef, windowResource_, UpdateWindowState, ResourceRef{BinaryFile::GetTypeStatic()}, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Is Open", bool, open_, UpdateWindowState, false, AM_DEFAULT);
}

void RmlUIComponent::OnNodeSet(Node* node)
{
    if (open_ && node != nullptr)
        OpenInternal();
    else
        CloseInternal();
}

void RmlUIComponent::SetOpen(bool open)
{
    if (!open)
        CloseInternal();
    else if (node_ != nullptr)
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

    if (windowResource_.name_.empty())
    {
        URHO3D_LOGERROR("UI document can be opened after setting resource path.");
        return;
    }

    RmlUI* ui = GetSubsystem<RmlUI>();
    document_ = ui->LoadDocument(windowResource_.name_);
}

void RmlUIComponent::CloseInternal()
{
    if (document_ == nullptr)
        return;

    document_->Close();
    document_ = nullptr;
}

void RmlUIComponent::OnDocumentClosed(StringHash, VariantMap& args)
{
    using namespace UIDocumentClosed;
    Rml::ElementDocument* document = reinterpret_cast<Rml::ElementDocument*>(args[P_DOCUMENT].GetVoidPtr());
    if (document_ == document)
    {
        document_ = nullptr;
        open_ = false;
    }
}

void RmlUIComponent::UpdateWindowState()
{
    SetOpen(open_);
}

}
