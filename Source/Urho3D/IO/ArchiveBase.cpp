// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../IO/ArchiveBase.h"

#include "../Core/StringUtils.h"

namespace Urho3D
{

void ArchiveBase::ReadBytesFromHexString(
    ea::string_view elementName, const ea::string& string, void* bytes, unsigned size)
{
    thread_local ByteVector tempBuffer;

    if (!HexStringToBuffer(tempBuffer, string))
        throw UnexpectedElementValueException(elementName);

    if (size != tempBuffer.size())
        throw UnexpectedElementValueException(elementName);

    ea::copy(tempBuffer.begin(), tempBuffer.end(), static_cast<unsigned char*>(bytes));
}
}
