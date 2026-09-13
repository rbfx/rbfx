// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/IO/AbstractFile.h"
#include "Urho3D/IO/MountPoint.h"
#include "Urho3D/IO/MemoryBuffer.h"

namespace Urho3D
{

/// Lightweight mount point that provides read-only access to the externally managed memory.
class URHO3D_API MountedExternalMemory : public MountPoint
{
    URHO3D_OBJECT(MountedExternalMemory, MountPoint)

public:
    explicit MountedExternalMemory(Context* context, ea::string_view scheme);

    void LinkMemory(ea::string_view fileName, MemoryBuffer memory);
    void LinkMemory(ea::string_view fileName, ea::string_view content);
    void UnlinkMemory(ea::string_view fileName);

    void SendFileChangedEvent(ea::string_view fileName);

    /// Implement MountPoint.
    /// @{
    bool AcceptsScheme(const ea::string& scheme) const override;
    bool Exists(const FileIdentifier& fileName) const override;
    AbstractFilePtr OpenFile(const FileIdentifier& fileName, FileMode mode) override;

    const ea::string& GetName() const override;

    void Scan(ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter,
        ScanFlags flags) const override;
    /// @}

private:
    ea::string scheme_;
    ea::unordered_map<ea::string, MemoryBuffer> files_{};
};

} // namespace Urho3D
