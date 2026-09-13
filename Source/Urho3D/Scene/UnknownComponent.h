// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Scene/Component.h"

namespace Urho3D
{

/// Placeholder for allowing unregistered components to be loaded & saved along with scenes.
class URHO3D_API UnknownComponent : public Component
{
    URHO3D_OBJECT(UnknownComponent, Component);

public:
    /// Construct.
    explicit UnknownComponent(Context* context);

    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Load from binary data. Return true if successful.
    bool Load(Deserializer& source) override;
    /// Load from XML data. Return true if successful.
    bool LoadXML(const XMLElement& source) override;
    /// Load from JSON data. Return true if successful.
    bool LoadJSON(const JSONValue& source) override;
};

}
