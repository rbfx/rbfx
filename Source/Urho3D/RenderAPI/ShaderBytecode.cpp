// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/RenderAPI/ShaderBytecode.h"

#include "Urho3D/IO/ArchiveSerialization.h"
#include "Urho3D/IO/BinaryArchive.h"

namespace Urho3D
{

void ShaderBytecode::SerializeInBlock(Archive& archive)
{
    const unsigned version = archive.SerializeVersion(Version);
    if (version != Version)
        throw ArchiveException("Compiled shader version is outdated, cached shader is ignored");

    SerializeValue(archive, "type", type_);
    SerializeValue(archive, "mime", mime_);
    SerializeVectorAsBytes(archive, "bytecode", bytecode_);

    SerializeVectorAsObjects(archive, "vertexAttributes", vertexAttributes_, "vertexAttribute",
        [&](Archive& archive, const char* name, VertexShaderAttribute& value)
    {
        auto block = archive.OpenUnorderedBlock(name);
        SerializeValue(archive, "semantic", value.semantic_);
        SerializeValue(archive, "semanticIndex", value.semanticIndex_);
        SerializeValue(archive, "inputIndex", value.inputIndex_);
    });
}

bool ShaderBytecode::SaveToFile(Serializer& dest) const
{
    // clang-format off
    const bool saved = ConsumeArchiveException([&]
    {
        BinaryOutputArchive archive{Context::GetInstance(), dest};
        SerializeValue(archive, "compiledShaderVariation", const_cast<ShaderBytecode&>(*this));
    }, false);
    // clang-format on

    return saved;
}

bool ShaderBytecode::LoadFromFile(Deserializer& source)
{
    // clang-format off
    const bool loaded = ConsumeArchiveException([&]
    {
        BinaryInputArchive archive{Context::GetInstance(), source};
        SerializeValue(archive, "compiledShaderVariation", *this);
    }, false);
    // clang-format on

    if (!loaded)
        *this = {};

    return loaded;
}

} // namespace Urho3D
