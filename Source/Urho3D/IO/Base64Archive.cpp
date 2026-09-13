// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/IO/Base64Archive.h"

#include "Urho3D/Core/StringUtils.h"

namespace Urho3D
{

Base64OutputArchive::Base64OutputArchive(Context* context)
    : BinaryOutputArchive(context, static_cast<VectorBuffer&>(*this))
{
}

ea::string Base64OutputArchive::GetBase64() const
{
    return EncodeBase64(GetBuffer());
}

Base64InputArchive::Base64InputArchive(Context* context, const ea::string& base64)
    : VectorBuffer(DecodeBase64(base64))
    , BinaryInputArchive(context, static_cast<VectorBuffer&>(*this))
{
}

}
