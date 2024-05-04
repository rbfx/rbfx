// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Scene/Serializable.h>

namespace Urho3D
{

/// Base class for serializable resource that uses Archive serialization.
class URHO3D_API SerializableResource : public SimpleResource
{
    URHO3D_OBJECT(SerializableResource, SimpleResource)

public:
    /// Construct.
    explicit SerializableResource(Context* context);
    /// Destruct.
    ~SerializableResource() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Override of SerializeInBlock.
    void SerializeInBlock(Archive& archive) override;

    /// Get resource value.
    Serializable* GetValue() const;

    /// Set value of the resource.
    void SetValue(Serializable* serializable);

private:
    SharedPtr<Serializable> value_;
};
}
