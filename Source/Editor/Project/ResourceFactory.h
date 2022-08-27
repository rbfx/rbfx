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

/// Interface of file and folder factory.
class ResourceFactory : public Object
{
    URHO3D_OBJECT(ResourceFactory, Object);

public:
    using CheckResult = ea::pair<bool, ea::string>;
    using FileNameChecker = ea::function<CheckResult(const ea::string& filePath, const ea::string& fileName)>;

    ResourceFactory(Context* context, int group, const ea::string& title);

    virtual bool IsEnabled(const FileSystemEntry& parentEntry) const { return true; }

    virtual void Open(const ea::string& baseFilePath, const ea::string baseResourcePath) = 0;
    virtual void Render(const FileNameChecker& checker, bool& canCommit, bool& shouldCommit) = 0;
    virtual void CommitAndClose() = 0;
    virtual void DiscardAndClose() {}

    int GetGroup() const { return group_; }
    const ea::string& GetTitle() const { return title_; }

    /// Order factories by group and name, usually for menu rendering.
    static bool Compare(const SharedPtr<ResourceFactory>& lhs, const SharedPtr<ResourceFactory>& rhs);

private:
    const int group_{};
    const ea::string title_;
};

/// Base implementation of ResourceFactory.
class BaseResourceFactory : public ResourceFactory
{
    URHO3D_OBJECT(BaseResourceFactory, ResourceFactory);

public:
    BaseResourceFactory(Context* context, int group, const ea::string& title);

    virtual ea::string GetDefaultFileName() const = 0;
    virtual bool IsFileNameEditable() const { return true; }
    virtual void RenderAuxilary() {}

    /// Implement ResourceFactory.
    /// @{
    void Open(const ea::string& baseFilePath, const ea::string baseResourcePath) override;
    void Render(const FileNameChecker& checker, bool& canCommit, bool& shouldCommit) override;
    void CommitAndClose() override;
    /// @}

protected:
    const ea::string& GetFinalFilePath() const { return baseFilePath_; }
    const ea::string& GetFinalResourcePath() const { return baseResourcePath_; }

    ea::string GetFinalFileName() const;
    ea::string GetFinalResourceName() const;

private:
    ea::string baseFilePath_;
    ea::string baseResourcePath_;

    ea::string localFileName_;
    ea::string newResourcePath_;

    bool selectFileNameInput_{};
};

/// Simple implementation of ResourceFactory.
class SimpleResourceFactory : public BaseResourceFactory
{
    URHO3D_OBJECT(SimpleResourceFactory, BaseResourceFactory);

public:
    using Callback = ea::function<void(const ea::string& fileName, const ea::string& resourceName)>;

    SimpleResourceFactory(Context* context,
        int group, const ea::string& title, const ea::string& fileName, const Callback& callback);

    /// Overridable interface
    /// @{
    ea::string GetDefaultFileName() const override { return fileName_; };
    void CommitAndClose() override;
    /// @}

private:
    const ea::string fileName_;
    Callback callback_;
};

}
