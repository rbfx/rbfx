// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../IO/BinaryArchive.h"
#include "../IO/VectorBuffer.h"

namespace Urho3D
{

/// Base64 output archive.
class URHO3D_API Base64OutputArchive : private VectorBuffer, public BinaryOutputArchive
{
public:
    Base64OutputArchive(Context* context);

    /// Return base64-encoded result.
    ea::string GetBase64() const;
};

/// Base64 input archive.
class URHO3D_API Base64InputArchive : private VectorBuffer, public BinaryInputArchive
{
public:
    Base64InputArchive(Context* context, const ea::string& base64);
};

}
