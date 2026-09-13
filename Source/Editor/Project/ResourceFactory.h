// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
