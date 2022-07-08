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

#include <Urho3D/Core/Object.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/XMLFile.h>

#include <EASTL/priority_queue.h>
#include <EASTL/string.h>

namespace Urho3D
{

/// Base class for project-wide requests. Should be used from main thread only!
class ProjectRequest : public Object
{
    URHO3D_OBJECT(ProjectRequest, Object);

public:
    using Callback = ea::function<void()>;

    explicit ProjectRequest(Context* context);

    /// Queue callback with priority that can be used to process request.
    void QueueProcessCallback(const Callback& callback, int priority = 0);
    /// Invoke callback with highest priority.
    void InvokeProcessCallback();

private:
    struct CallbackDesc
    {
        Callback callback_{};
        int priority_{};

        bool operator<(const CallbackDesc& other) const;
    };

    ea::priority_queue<CallbackDesc> callbacks_;
};

/// Helper to describe file resource in the engine.
class FileResourceDesc
{
public:
    FileResourceDesc() = default;
    FileResourceDesc(Context* context, const ea::string& resourceName);

    Context* GetContext() const { return context_; }
    bool IsValidFile() const { return !fileName_.empty(); }

    SharedPtr<File> GetBinaryFile() const;
    SharedPtr<XMLFile> GetXMLFile() const;
    SharedPtr<JSONFile> GetJSONFile() const;

    /// Return type hint from the file itself.
    /// - Root element name of XML.
    /// - Empty otherwise.
    ea::string GetTypeHint() const;

    /// Return properties.
    /// @{
    const ea::string& GetResourceName() const { return resourceName_; }
    const ea::string& GetFileName() const { return fileName_; }
    /// @}

private:
    Context* context_{};

    ea::string resourceName_;
    ea::string fileName_;

    mutable SharedPtr<XMLFile> xmlFile_;
    mutable SharedPtr<JSONFile> jsonFile_;
};

/// Request to open resource.
class OpenResourceRequest : public ProjectRequest, public FileResourceDesc
{
    URHO3D_OBJECT(OpenResourceRequest, ProjectRequest);

public:
    explicit OpenResourceRequest(Context* context, const ea::string& resourceName);
};

/// Request to inspect one or more resources.
class InspectResourceRequest : public ProjectRequest
{
    URHO3D_OBJECT(InspectResourceRequest, ProjectRequest);

public:
    explicit InspectResourceRequest(Context* context, const ea::vector<ea::string>& resourceNames);

    const ea::vector<FileResourceDesc>& GetResources() const { return resourceDescs_; }
    StringVector GetSortedResourceNames() const;

private:
    ea::vector<FileResourceDesc> resourceDescs_;
};

}