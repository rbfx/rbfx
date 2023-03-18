//
// Copyright (c) 2022-2023 the Urho3D project.
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

#include "FileWatcher.h"
#include "../IO/MountPoint.h"
#include "../Container/FlagSet.h"

namespace Urho3D
{

/// Stores files of a directory tree sequentially for convenient access.
class URHO3D_API MountedDirectory : public MountPoint
{
    URHO3D_OBJECT(MountedDirectory, MountPoint);

public:
    /// Construct.
    explicit MountedDirectory(Context* context);
    /// Construct and open.
    MountedDirectory(Context* context, const ea::string& directory, ea::string scheme = EMPTY_STRING);
    /// Destruct.
    ~MountedDirectory() override;

    /// Implement MountPoint.
    /// @{
    bool AcceptsScheme(const ea::string& scheme) const override;
    bool Exists(const FileIdentifier& fileName) const override;
    AbstractFilePtr OpenFile(const FileIdentifier& fileName, FileMode mode) override;

    const ea::string& GetName() const override { return name_; }

    ea::string GetAbsoluteNameFromIdentifier(const FileIdentifier& fileName) const override;
    FileIdentifier GetIdentifierFromAbsoluteName(const ea::string& absoluteFileName) const override;

    void Scan(ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter,
        unsigned flags, bool recursive) const override;
    /// @}


    /// Get mounted directory path.
    const ea::string& GetDirectory() const { return directory_; }

protected:
    ea::string SanitizeDirName(const ea::string& name) const;

    /// Start file watcher.
    void StartWatching() override;
    /// Stop file watcher.
    void StopWatching() override;

private:
    /// Handle begin frame event. Automatic resource reloads are processed here.
    void HandleBeginFrame(StringHash eventType, VariantMap& eventData);

private:
    /// Expected file locator scheme.
    const ea::string scheme_;
    /// Target directory.
    const ea::string directory_;
    /// Name of the mount point.
    const ea::string name_;
    /// File watcher for resource directory, if automatic reloading enabled.
    SharedPtr<FileWatcher> fileWatcher_;
};

} // namespace Urho3D
