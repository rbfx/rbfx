// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/ByteVector.h"
#include "Urho3D/IO/AbstractFile.h"
#include "Urho3D/IO/Archive.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"

namespace Urho3D
{

struct ShaderBytecode
{
    /// Version of the shader bytecode format. Increment when serialization format changes.
    static const unsigned Version = 1;

    ShaderType type_{};

    ea::string mime_{};
    ByteVector bytecode_{};

    /// Vertex input layout, if applicable.
    VertexShaderAttributeVector vertexAttributes_{};

    void SerializeInBlock(Archive& archive);

    bool SaveToFile(Serializer& dest) const;
    bool LoadFromFile(Deserializer& source);

    bool IsEmpty() const { return bytecode_.empty(); }
};

} // namespace Urho3D
