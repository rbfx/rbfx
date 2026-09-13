// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once


#include "Urho3D/Container/Ptr.h"
#include "Urho3D/IO/FileIdentifier.h"

#include <RmlUi/Core/FileInterface.h>

#include <unordered_set>

namespace Urho3D
{

class Context;

namespace Detail
{

class URHO3D_API RmlFile : public Rml::FileInterface
{
public:
    /// Construct.
    explicit RmlFile(Context* context);

    /// Opens a file.
    Rml::FileHandle Open(const Rml::String& path) override;
    /// Closes a previously opened file.
    void Close(Rml::FileHandle file) override;
    /// Reads data from a previously opened file.
    size_t Read(void* buffer, size_t size, Rml::FileHandle file) override;
    /// Seeks to a point in a previously opened file.
    bool Seek(Rml::FileHandle file, long offset, int origin) override;
    /// Returns the current position of the file pointer.
    size_t Tell(Rml::FileHandle file) override;
    /// Returns the length of the file.
    size_t Length(Rml::FileHandle file) override;

    /// Returns true if file was opened since last call to ClearOpenedFiles().
    bool IsResourceLoaded(const ea::string& resourceName);
    /// Add resource to the set of opened files.
    void AddResourceLoaded(const ea::string& resourceName);
    /// Clear a set of opened files.
    void ClearLoadedResources() { loadedResources_.clear(); }

private:
    /// Context pointer.
    WeakPtr<Context> context_;
    /// A set of loaded files. Used to trigger UI reloads when resource cache reloads a modified file.
    ea::unordered_set<ea::string> loadedResources_;
};

}   // namespace Detail

}   // namespace Urho3D
