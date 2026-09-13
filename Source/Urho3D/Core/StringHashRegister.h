// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <EASTL/unique_ptr.h>

#include "Urho3D/Container/Str.h"
#include "Urho3D/Core/Variant.h"
#include "Urho3D/Math/StringHash.h"

namespace Urho3D
{

class Mutex;
class StringHash;

/// Helper class used for StringHash reversing.
class URHO3D_API StringHashRegister
{
public:
    /// Construct. threadSafe controls whether the RegisterString and GetStringCopy are thread-safe.
    StringHashRegister(bool threadSafe);
    /// Destruct.
    ~StringHashRegister();

    /// Register string for hash reverse mapping. Could be used from StringHash ctor.
    StringHash RegisterString(const StringHash& hash, ea::string_view string);
    /// Register string for hash reverse mapping.
    StringHash RegisterString(ea::string_view string);
    /// Return string for given StringHash. Return empty string if not found.
    ea::string GetStringCopy(const StringHash& hash) const;
    /// Return whether the string in contained in the register.
    bool Contains(const StringHash& hash) const;

    /// Return String for given StringHash. Return value is unsafe to use if RegisterString is called from other threads.
    const ea::string& GetString(const StringHash& hash) const;
    /// Return map of hashes. Return value is unsafe to use if RegisterString is called from other threads.
    const StringMap& GetInternalMap() const { return map_; }

private:
    /// Hash to string map.
    StringMap map_;
    /// Mutex.
    ea::unique_ptr<Mutex> mutex_;
};

}
