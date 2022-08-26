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

#include <Urho3D/Utility/FileSystemReflection.h>

#include <EASTL/functional.h>

namespace Urho3D
{

/// Base class for utility to create new files.
class ResourceFactory : public Object
{
    URHO3D_OBJECT(ResourceFactory, Object);

public:
    ResourceFactory(Context* context, int group, const ea::string& title);

    /// Overridable interface
    /// @{
    virtual ea::string GetFileName() const = 0;
    virtual bool IsEnabled(const FileSystemEntry& parentEntry) const { return true; }
    virtual void BeginCreate() {}
    virtual void Render() {}
    virtual void EndCreate(const ea::string& fileName, const ea::string& resourceName) = 0;
    /// @}

    int GetGroup() const { return group_; }
    const ea::string& GetTitle() const { return title_; }

    /// Order factories by group and name, usually for menu rendering.
    static bool Compare(const SharedPtr<ResourceFactory>& lhs, const SharedPtr<ResourceFactory>& rhs);

private:
    const int group_{};
    const ea::string title_;
};

/// Simple implementation of ResourceFactory.
class SimpleResourceFactory : public ResourceFactory
{
    URHO3D_OBJECT(SimpleResourceFactory, ResourceFactory);

public:
    using Callback = ea::function<void(const ea::string& fileName, const ea::string& resourceName)>;

    SimpleResourceFactory(Context* context,
        int group, const ea::string& title, const ea::string& fileName, const Callback& callback);

    /// Overridable interface
    /// @{
    ea::string GetFileName() const override { return fileName_; };
    void EndCreate(const ea::string& fileName, const ea::string& resourceName) override;
    /// @}

private:
    const ea::string fileName_;
    Callback callback_;
};

}
