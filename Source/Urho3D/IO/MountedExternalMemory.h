//
// Copyright (c) 2023-2023 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//


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
