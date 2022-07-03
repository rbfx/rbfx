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

#include "../Project/ProjectRequest.h"

#include <Urho3D/Resource/ResourceCache.h>

namespace Urho3D
{

bool ProjectRequest::CallbackDesc::operator<(const CallbackDesc& other) const
{
    return priority_ < other.priority_;
}

ProjectRequest::ProjectRequest(Context* context)
    : Object(context)
{
}

void ProjectRequest::QueueProcessCallback(const Callback& callback, int priority)
{
    callbacks_.emplace(CallbackDesc{callback, priority});
}

void ProjectRequest::InvokeProcessCallback()
{
    if (!callbacks_.empty())
    {
        const auto& callback = callbacks_.top().callback_;
        callback();
    }
}

SharedPtr<OpenResourceRequest> OpenResourceRequest::Create(Context* context, const ea::string& resourceName, bool useInspector)
{
    SharedPtr<OpenResourceRequest> request{new OpenResourceRequest(context, resourceName, useInspector)};
    return request->IsValid() ? request : nullptr;
}

OpenResourceRequest::OpenResourceRequest(Context* context, const ea::string& resourceName, bool useInspector)
    : ProjectRequest(context)
    , resourceName_(resourceName)
    , useInspector_(useInspector)
{
    auto cache = context_->GetSubsystem<ResourceCache>();

    file_ = cache->GetFile(resourceName);
    if (file_)
        fileName_ = file_->GetAbsoluteName();
}

SharedPtr<XMLFile> OpenResourceRequest::GetXMLFile() const
{
    if (!xmlFile_ && resourceName_.ends_with(".xml", false))
    {
        auto cache = context_->GetSubsystem<ResourceCache>();
        auto file = cache->GetFile(resourceName_);
        xmlFile_ = MakeShared<XMLFile>(context_);
        xmlFile_->Load(*file);
    }
    return xmlFile_;
}

SharedPtr<JSONFile> OpenResourceRequest::GetJSONFile() const
{
    if (!jsonFile_ && resourceName_.ends_with(".json", false))
    {
        auto cache = context_->GetSubsystem<ResourceCache>();
        auto file = cache->GetFile(resourceName_);
        jsonFile_ = MakeShared<JSONFile>(context_);
        jsonFile_->Load(*file);
    }
    return jsonFile_;
}

ea::string OpenResourceRequest::GetTypeHint() const
{
    if (const auto xmlFile = GetXMLFile())
        return xmlFile->GetRoot().GetName();
    return EMPTY_STRING;
}

}
