// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include <Urho3D/Resource/SerializableResource.h>

namespace Urho3D
{

SerializableResource::SerializableResource(Context* context)
    : SimpleResource(context)
{
}

SerializableResource::~SerializableResource() = default;

void SerializableResource::RegisterObject(Context* context)
{
    context->AddFactoryReflection<SerializableResource>();
}

void SerializableResource::SerializeInBlock(Archive& archive)
{
    ArchiveBlock block = archive.OpenUnorderedBlock("resource");
    if (archive.IsInput())
    {
        ea::string typeName;
        SerializeOptionalValue(archive, "type", typeName);
        if (!typeName.empty())
        {
            ArchiveBlock valueBlock = archive.OpenUnorderedBlock("value");
            value_.DynamicCast(context_->CreateObject(typeName));
            if (value_)
            {
                value_->SerializeInBlock(archive);
            }
            else
            {
                throw ArchiveException("Failed to create object of type '{}'", typeName);
            }
        }
    }
    else
    {
        if (value_)
        {
            auto typeName = value_->GetTypeName();
            SerializeValue(archive, "type", typeName);
            ArchiveBlock valueBlock = archive.OpenUnorderedBlock("value");
            value_->SerializeInBlock(archive);
        }
    }
}

Serializable* SerializableResource::GetValue() const
{
    return value_;
}

void SerializableResource::SetValue(Serializable* serializable)
{
    value_ = serializable;
}

} // namespace Urho3D
