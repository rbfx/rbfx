// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../RmlUI/RmlUIComponent.h"

#include <RmlUi/Core/DataModelHandle.h>

namespace Urho3D
{

struct RmlSerializableAttribute;

/// UI widget to inspect contents of Serializable.
class URHO3D_API RmlSerializableInspector : public RmlUIComponent
{
    URHO3D_OBJECT(RmlSerializableInspector, RmlUIComponent);

public:
    explicit RmlSerializableInspector(Context* context);
    ~RmlSerializableInspector() override;
    static void RegisterObject(Context* context);

    /// Connect widget to Serializable.
    void Connect(Serializable* serializable);

protected:
    /// RmlUIComponent implementation
    /// @{
    void Update(float timeStep) override;
    void OnDataModelInitialized() override;
    /// @}

    WeakPtr<Serializable> serializable_;
    Rml::String type_;
    Rml::Vector<RmlSerializableAttribute> attributes_;
};

}
