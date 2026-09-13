// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Actions/ShaderParameter.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Actions/FiniteTimeActionState.h"
#include "Urho3D/Graphics/Material.h"

namespace Urho3D
{
namespace Actions
{

namespace
{
class ShaderParameterFromToState : public FiniteTimeActionState
{
    Variant from_;
    Variant to_;
    ea::string name_;

public:
    ShaderParameterFromToState(ShaderParameterFromTo* action, Object* target)
        : FiniteTimeActionState(action, target)
        , from_(action->GetFrom())
        , to_(action->GetTo())
        , name_(action->GetName())
    {
    }

    void Update(float time) override
    {
        auto* material = GetTarget()->Cast<Material>();
        if (material)
        {
            material->SetShaderParameter(name_, from_.Lerp(to_, time));
        }
    }
};

class ShaderParameterToState : public FiniteTimeActionState
{
    Variant from_;
    Variant to_;
    ea::string name_;

public:
    ShaderParameterToState(ShaderParameterTo* action, Object* target)
        : FiniteTimeActionState(action, target)
        , to_(action->GetTo())
        , name_(action->GetName())
    {
        const auto* material = GetTarget()->Cast<Material>();
        if (material)
        {
            from_ = material->GetShaderParameter(name_);
            if (from_.GetType() != to_.GetType())
            {
                from_ = to_;
            }
        }
    }

    void Update(float time) override
    {
        auto* material = GetTarget()->Cast<Material>();
        if (material)
        {
            material->SetShaderParameter(name_, from_.Lerp(to_, time));
        }
    }
};


} // namespace

/// --------------------------------------------------
/// Construct.
ShaderParameterAction::ShaderParameterAction(Context* context)
    : BaseClassName(context)
{
}

// Get shader parameter name
void ShaderParameterAction::SetName(ea::string_view name) { name_ = name; }

/// Serialize content from/to archive. May throw ArchiveException.
void ShaderParameterAction::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeValue(archive, "name", name_);
}


/// --------------------------------------------------
/// Construct.
ShaderParameterTo::ShaderParameterTo(Context* context)
    : BaseClassName(context)
{
}

// Get "to" value.
void ShaderParameterTo::SetTo(const Variant& variant) { to_ = variant; }

/// Serialize content from/to archive. May throw ArchiveException.
void ShaderParameterTo::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "to", to_, Variant::EMPTY);
}

/// Create new action state from the action.
SharedPtr<ActionState> ShaderParameterTo::StartAction(Object* target)
{
    return MakeShared<ShaderParameterToState>(this, target);
}

/// --------------------------------------------------
/// Construct.
ShaderParameterFromTo::ShaderParameterFromTo(Context* context)
    : BaseClassName(context)
{
}

// Set "from" value.
void ShaderParameterFromTo::SetFrom(const Variant& variant) { from_ = variant; }

/// Create reversed action.
SharedPtr<FiniteTimeAction> ShaderParameterFromTo::Reverse() const
{
    auto result = MakeShared<ShaderParameterFromTo>(context_);
    result->SetDuration(GetDuration());
    result->SetName(GetName());
    result->SetFrom(GetTo());
    result->SetTo(from_);
    return result;
}

/// Serialize content from/to archive. May throw ArchiveException.
void ShaderParameterFromTo::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "from", from_, Variant::EMPTY);
}

/// Create new action state from the action.
SharedPtr<ActionState> ShaderParameterFromTo::StartAction(Object* target)
{
    return MakeShared<ShaderParameterFromToState>(this, target);
}

} // namespace Actions
} // namespace Urho3D
