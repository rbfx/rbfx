// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "FiniteTimeAction.h"

namespace Urho3D
{
namespace Actions
{
const ea::string_view POSITION_ATTRIBUTE{"Position"};
const ea::string_view ROTATION_ATTRIBUTE{"Rotation"};
const ea::string_view SCALE_ATTRIBUTE{"Scale"};
const ea::string_view ISVISIBLE_ATTRIBUTE{"Is Visible"};
const ea::string_view ISENABLED_ATTRIBUTE{"Is Enabled"};

/// Finite time action.
class URHO3D_API AttributeAction : public FiniteTimeAction
{
    URHO3D_OBJECT(AttributeAction, FiniteTimeAction)
public:
    /// Construct.
    AttributeAction(Context* context);
    /// Construct.
    AttributeAction(Context* context, ea::string_view animatedAttribute);

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

    /// Set action duration.
    void SetAttributeName(ea::string_view animatedAttribute);
    /// Get action duration.
    virtual const ea::string& GetAttributeName() const;
protected:
    AttributeInfo* GetAttribute(Object* target);

private:
    ea::string animatedAttribute_{};
};

} // namespace Actions
} // namespace Urho3D
