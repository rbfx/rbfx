// Copyright (c) 2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "BaseAction.h"
#include "../Resource/Resource.h"
#include "../IO/Archive.h"

namespace Urho3D
{

class XMLFile;

/// Action as resource
class URHO3D_API ActionSet : public Resource
{
    URHO3D_OBJECT(ActionSet, Resource)

public:
    /// Construct.
    explicit ActionSet(Context* context);
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;

    /// Save resource. Return true if successful.
    bool Save(Serializer& dest) const override;
    /// Serialize from/to archive. Return true if successful.
    void SerializeInBlock(Archive& archive) override;

    /// Get action
    Actions::BaseAction* GetDefaultAction() const { return defaultAction_; }
    /// Set action
    void SetDefaultAction(Actions::BaseAction* action);

private:
    /// Root action.
    SharedPtr<Actions::BaseAction> defaultAction_;
};

} // namespace Urho3D
